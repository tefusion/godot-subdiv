#include "subdivider.hpp"

#include "godot_cpp/classes/mesh_data_tool.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/builtin_types.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"

//debug
// #include <chrono>
// using namespace std::chrono;

using namespace OpenSubdiv;
typedef Far::TopologyDescriptor Descriptor;

Subdivider::TopologyData::TopologyData(const Array &p_mesh_arrays, int32_t p_format, int32_t p_face_verts) {
	vertex_array = p_mesh_arrays[TopologyDataMesh::ARRAY_VERTEX];
	index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_INDEX];
	if (p_format & Mesh::ARRAY_FORMAT_TEX_UV) {
		uv_array = p_mesh_arrays[TopologyDataMesh::ARRAY_TEX_UV];
		uv_index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_UV_INDEX];
	}
	if ((p_format & Mesh::ARRAY_FORMAT_BONES) && (p_format & Mesh::ARRAY_FORMAT_WEIGHTS)) {
		bones_array = p_mesh_arrays[TopologyDataMesh::ARRAY_BONES];
		weights_array = p_mesh_arrays[TopologyDataMesh::ARRAY_WEIGHTS];
	}

	vertex_count_per_face = p_face_verts;
	index_count = index_array.size();
	face_count = index_array.size() / vertex_count_per_face;
	vertex_count = vertex_array.size();
	uv_count = uv_index_array.size();
	bone_count = bones_array.size();
	weight_count = weights_array.size();
}

struct Vertex {
	void Clear() { x = y = z = 0; }
	void AddWithWeight(Vertex const &src, float weight) {
		x += weight * src.x;
		y += weight * src.y;
		z += weight * src.z;
	}
	float x, y, z;
};

struct VertexUV {
	void Clear() {
		u = v = 0.0f;
	}

	void AddWithWeight(VertexUV const &src, float weight) {
		u += weight * src.u;
		v += weight * src.v;
	}

	// Basic 'uv' layout channel
	float u, v;
};

struct VertexWeights {
	void Clear() {
		for (int i = 0; i < weights.size(); i++) {
			weights[i] = 0;
		}
	}

	void AddWithWeight(VertexWeights const &src, float weight) {
		for (int i = 0; i < weights.size(); i++) {
			weights[i] += src.weights[i] * weight;
		}
	}

	PackedFloat32Array weights;
};

Descriptor Subdivider::_create_topology_descriptor(Vector<int> &subdiv_face_vertex_count, Descriptor::FVarChannel *channels, const int32_t p_format) {
	const bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;

	Descriptor desc;
	desc.numVertices = topology_data.vertex_count;
	desc.numFaces = topology_data.face_count;
	desc.vertIndicesPerFace = topology_data.index_array.ptr();

	subdiv_face_vertex_count = _get_face_vertex_count();

	desc.numVertsPerFace = subdiv_face_vertex_count.ptr();

	int num_channels = 0;
	if (use_uv) {
		channels[Channels::UV].numValues = topology_data.uv_count;
		channels[Channels::UV].valueIndices = topology_data.uv_index_array.ptr();
		num_channels = 1;
	}

	desc.numFVarChannels = num_channels;
	desc.fvarChannels = channels;

	return desc;
}

Far::TopologyRefiner *Subdivider::_create_topology_refiner(const int32_t p_level, const int32_t p_format) {
	const bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;

	//create descriptor,
	Vector<int> subdiv_face_vertex_count;
	int num_channels = 0;
	if (use_uv) {
		num_channels = 1;
	}

	Descriptor::FVarChannel *channels = new Descriptor::FVarChannel[num_channels];
	Descriptor desc = _create_topology_descriptor(subdiv_face_vertex_count, channels, p_format);

	Sdc::SchemeType type = _get_refiner_type();
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

	Far::TopologyRefiner *refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
	delete channels;
	ERR_FAIL_COND_V(!refiner, nullptr);

	Far::TopologyRefiner::UniformOptions refine_options(p_level);
	refine_options.fullTopologyInLastLevel = true;
	refiner->RefineUniform(refine_options);

	topology_data.vertex_count = refiner->GetNumVerticesTotal();
	if (use_uv) {
		topology_data.uv_count = refiner->GetNumFVarValuesTotal(Channels::UV);
	}

	return refiner;
}
void Subdivider::_create_subdivision_vertices(Far::TopologyRefiner *refiner, const int p_level, const int32_t p_format) {
	const bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;
	const bool use_bones = (p_format & Mesh::ARRAY_FORMAT_BONES) && (p_format & Mesh::ARRAY_FORMAT_WEIGHTS);

	int original_vertex_count = topology_data.vertex_array.size();
	topology_data.vertex_array.resize(topology_data.vertex_count);

	// Interpolate vertex primvar data
	Far::PrimvarRefiner primvar_refiner(*refiner);
	//vertices
	Vertex *src = (Vertex *)topology_data.vertex_array.ptr();
	for (int level = 0; level < p_level; ++level) {
		Vertex *dst = src + refiner->GetLevel(level).GetNumVertices();
		primvar_refiner.Interpolate(level + 1, src, dst);
		src = dst;
	}

	if (use_uv) {
		topology_data.uv_array.resize(topology_data.uv_count);
		VertexUV *src_uv = (VertexUV *)topology_data.uv_array.ptr();
		for (int level = 0; level < p_level; ++level) {
			VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(Channels::UV);
			primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, Channels::UV);
			src_uv = dst_uv;
		}
	}

	if (use_bones) {
		//What happens here is just create weights array that contain ALL weights
		//indexed to bones per vertex to allow interpolating them and afterwards pick the 4 highest weights

		int highest_bone_index = 0;

		for (int i = 0; i < topology_data.bones_array.size(); i++) {
			if (topology_data.bones_array[i] > highest_bone_index) {
				highest_bone_index = topology_data.bones_array[i];
			}
		}

		//create array with all weights per vertex
		Vector<PackedFloat32Array> all_vertex_bone_weights; //will contain all weights per vertex indexed to bones
		all_vertex_bone_weights.resize(topology_data.vertex_count);

		//resize to fit all weights
		for (int vertex_index = 0; vertex_index < topology_data.vertex_count; vertex_index++) {
			all_vertex_bone_weights.write[vertex_index].resize(highest_bone_index + 1);
		}

		//fill in already existing weights
		for (int vertex_index = 0; vertex_index < original_vertex_count; vertex_index++) {
			for (int weight_index = 0; weight_index < 4; weight_index++) {
				if (topology_data.weights_array[vertex_index * 4 + weight_index] != 0.0f) {
					int bone_index = topology_data.bones_array[vertex_index * 4 + weight_index];
					all_vertex_bone_weights.write[vertex_index][bone_index] = topology_data.weights_array[vertex_index * 4 + weight_index];
				}
			}
		}

		//interpolate with the custom class (just iterates over a packedfloatarray)
		VertexWeights *src_weights = (VertexWeights *)all_vertex_bone_weights.ptrw();
		for (int level = 0; level < p_level; ++level) {
			VertexWeights *dst_weights = src_weights + refiner->GetLevel(level).GetNumVertices();
			primvar_refiner.Interpolate(level + 1, src_weights, dst_weights);
			src_weights = dst_weights;
		}

		//select 4 highest weights and set them in topology_data
		topology_data.bones_array.resize(topology_data.vertex_count * 4);
		topology_data.weights_array.resize(topology_data.vertex_count * 4);
		for (int vertex_index = 0; vertex_index < topology_data.vertex_count; vertex_index++) {
			int weight_indices[4] = { -1, -1, -1, -1 };
			const PackedFloat32Array &vertex_bones_weights = all_vertex_bone_weights[vertex_index];

			for (int weight_index = 0; weight_index <= highest_bone_index; weight_index++) {
				if (vertex_bones_weights[weight_index] != 0 && (weight_indices[3] == -1 || vertex_bones_weights[weight_index] > vertex_bones_weights[weight_indices[3]])) {
					weight_indices[3] = weight_index;
					//move to right place, highest weight at position 0
					for (int i = 2; i >= 0; i--) {
						if (weight_indices[i] == -1 || vertex_bones_weights[weight_index] > vertex_bones_weights[weight_indices[i]]) {
							//swap
							weight_indices[i + 1] = weight_indices[i];
							weight_indices[i] = weight_index;
						} else {
							break;
						}
					}
				}
			}

			//save data in bones and weights array
			for (int result_weight_index = 0; result_weight_index < 4; result_weight_index++) {
				if (weight_indices[result_weight_index] == -1) { //happens if not 4 bones
					topology_data.bones_array[vertex_index * 4 + result_weight_index] = 0;
					topology_data.weights_array[vertex_index * 4 + result_weight_index] = 0;
				} else {
					topology_data.bones_array[vertex_index * 4 + result_weight_index] = weight_indices[result_weight_index];
					topology_data.weights_array[vertex_index * 4 + result_weight_index] = vertex_bones_weights[weight_indices[result_weight_index]];
				}
			}
		}
	}
}

Array Subdivider::get_subdivided_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	subdivide(p_arrays, p_level, p_format, calculate_normals);
	return _get_triangle_arrays();
}

Array Subdivider::get_subdivided_topology_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	ERR_FAIL_COND_V(p_level <= 0, Array());
	subdivide(p_arrays, p_level, p_format, calculate_normals);
	Array arr;
	arr.resize(TopologyDataMesh::ARRAY_MAX);
	arr[TopologyDataMesh::ARRAY_VERTEX] = topology_data.vertex_array;
	arr[TopologyDataMesh::ARRAY_NORMAL] = topology_data.normal_array;
	arr[TopologyDataMesh::ARRAY_TEX_UV] = topology_data.uv_array;
	arr[TopologyDataMesh::ARRAY_UV_INDEX] = topology_data.uv_index_array;
	arr[TopologyDataMesh::ARRAY_INDEX] = topology_data.index_array;
	arr[TopologyDataMesh::ARRAY_BONES] = topology_data.bones_array;
	arr[TopologyDataMesh::ARRAY_WEIGHTS] = topology_data.weights_array;
	return arr;
}

void Subdivider::subdivide(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	ERR_FAIL_COND(p_level < 0);
	const bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;
	const bool use_bones = (p_format & Mesh::ARRAY_FORMAT_BONES) && (p_format & Mesh::ARRAY_FORMAT_WEIGHTS);

	topology_data = TopologyData(p_arrays, p_format, _get_vertices_per_face_count());
	//if p_level not 0 subdivide mesh and store in topology_data again
	if (p_level != 0) {
		Far::TopologyRefiner *refiner = _create_topology_refiner(p_level, p_format);
		ERR_FAIL_COND_MSG(!refiner, "Refiner couldn't be created, numVertsPerFace array likely lost.");
		_create_subdivision_vertices(refiner, p_level, p_format);
		_create_subdivision_faces(refiner, p_level, p_format);
		//free memory

		delete refiner;
	}

	if (calculate_normals) {
		topology_data.normal_array = _calculate_smooth_normals(topology_data.vertex_array, topology_data.index_array);
	}
}

//TODO: virtual calls are not implemented yet in godot cpp (I think, it wasnt 2 weeks ago and didn't see any commit)
OpenSubdiv::Sdc::SchemeType Subdivider::_get_refiner_type() const {
	return Sdc::SchemeType::SCHEME_CATMARK;
}

void Subdivider::_create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
		const int32_t p_level, int32_t p_format) {
	const bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;
	const bool use_bones = (p_format & Mesh::ARRAY_FORMAT_BONES) && (p_format & Mesh::ARRAY_FORMAT_WEIGHTS);

	PackedInt32Array index_array;
	PackedInt32Array uv_index_array;

	Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
	int face_count_out = last_level.GetNumFaces();
	int uv_index_offset = use_uv ? topology_data.uv_count - last_level.GetNumFVarValues(Channels::UV) : -1;

	int vertex_index_offset = topology_data.vertex_count - last_level.GetNumVertices();
	for (int face_index = 0; face_index < face_count_out; ++face_index) {
		int parent_face_index = last_level.GetFaceParentFace(face_index);
		for (int level_index = p_level - 1; level_index > 0; --level_index) {
			Far::TopologyLevel const &prev_level = refiner->GetLevel(level_index);
			parent_face_index = prev_level.GetFaceParentFace(parent_face_index);
		}

		Far::ConstIndexArray face_vertices = last_level.GetFaceVertices(face_index);

		ERR_FAIL_COND(face_vertices.size() != topology_data.vertex_count_per_face);
		for (int face_vert_index = 0; face_vert_index < topology_data.vertex_count_per_face; face_vert_index++) {
			index_array.push_back(vertex_index_offset + face_vertices[face_vert_index]);
		}

		if (use_uv) {
			Far::ConstIndexArray face_uvs = last_level.GetFaceFVarValues(face_index, Channels::UV);
			for (int face_vert_index = 0; face_vert_index < topology_data.vertex_count_per_face; face_vert_index++) {
				uv_index_array.push_back(uv_index_offset + face_uvs[face_vert_index]);
			}
		}
	}
	topology_data.index_array = index_array;
	if (use_uv) {
		topology_data.uv_index_array = uv_index_array;
	}
}

PackedVector3Array Subdivider::_calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array) {
	PackedVector3Array normals;
	normals.resize(quad_vertex_array.size());
	for (int f = 0; f < quad_index_array.size(); f += topology_data.vertex_count_per_face) {
		// // We will use the first three verts to calculate a normal
		const Vector3 &p_point1 = quad_vertex_array[quad_index_array[f]];
		const Vector3 &p_point2 = quad_vertex_array[quad_index_array[f + 1]];
		const Vector3 &p_point3 = quad_vertex_array[quad_index_array[f + 2]];
		Vector3 normal_calculated = (p_point1 - p_point3).cross(p_point1 - p_point2);
		normal_calculated.normalize();
		for (int n_pos = f; n_pos < f + topology_data.vertex_count_per_face; n_pos++) {
			int vertexIndex = quad_index_array[n_pos];
			normals[vertexIndex] += normal_calculated;
		}
	}
	//normalized accumulated normals
	for (int vertex_index = 0; vertex_index < normals.size(); ++vertex_index) {
		normals[vertex_index].normalize();
	}
	return normals;
}

Array Subdivider::_get_triangle_arrays() const {
	return Array();
}

Vector<int> Subdivider::_get_face_vertex_count() const {
	return Vector<int>();
}

int32_t Subdivider::_get_vertices_per_face_count() const {
	return 0;
}
Array Subdivider::_get_direct_triangle_arrays() const {
	return Array();
}

void Subdivider::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_subdivided_arrays"), &Subdivider::get_subdivided_arrays);
	ClassDB::bind_method(D_METHOD("get_subdivided_topology_arrays"), &Subdivider::get_subdivided_topology_arrays);
}

Subdivider::Subdivider() {
}
Subdivider::~Subdivider() {
}
