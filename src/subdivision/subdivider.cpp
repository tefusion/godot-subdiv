#include "subdivider.hpp"

#include "godot_cpp/classes/mesh_data_tool.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
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
	if (p_format & Mesh::ARRAY_FORMAT_BONES && p_format & Mesh::ARRAY_FORMAT_WEIGHTS) {
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

struct VertexBones {
	void Clear() {
		b0 = b1 = b2 = b3 = 0;
	}

	void AddWithWeight(VertexBones const &src, float weight) {
		b0 = src.b0;
		b1 = src.b1;
		b2 = src.b2;
		b3 = src.b3;
	}

	int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
};
struct VertexWeights {
	void Clear() {
		w0 = w1 = w2 = w3 = 0.0f;
	}

	void AddWithWeight(VertexWeights const &src, float weight) {
		w0 += weight * src.w0;
		w1 += weight * src.w1;
		w2 += weight * src.w2;
		w3 += weight * src.w3;
	}

	float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.f;
};

Descriptor Subdivider::_create_topology_descriptor(const int num_channels, Vector<int> &subdiv_face_vertex_count, Descriptor::FVarChannel *channels) {
	Descriptor desc;
	desc.numVertices = topology_data.vertex_count;
	desc.numFaces = topology_data.face_count;
	desc.vertIndicesPerFace = topology_data.index_array.ptr();

	subdiv_face_vertex_count = _get_face_vertex_count();

	desc.numVertsPerFace = subdiv_face_vertex_count.ptr();

	if (num_channels > 0) {
		channels[Channels::UV].numValues = topology_data.uv_count;
		channels[Channels::UV].valueIndices = topology_data.uv_index_array.ptr();
		desc.numFVarChannels = num_channels;
		desc.fvarChannels = channels;
	}
	return desc;
}

Far::TopologyRefiner *Subdivider::_create_topology_refiner(const int32_t p_level, const int num_channels) {
	//create descriptor,
	Vector<int> subdiv_face_vertex_count;
	Descriptor::FVarChannel channels[num_channels];
	Descriptor desc = _create_topology_descriptor(num_channels, subdiv_face_vertex_count, channels);

	Sdc::SchemeType type = _get_refiner_type();
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

	Far::TopologyRefiner *refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
	//ERR_FAIL_COND(!refiner);
	ERR_FAIL_COND_V(!refiner, nullptr);

	Far::TopologyRefiner::UniformOptions refine_options(p_level);
	refine_options.fullTopologyInLastLevel = true;
	refiner->RefineUniform(refine_options);

	topology_data.vertex_count = refiner->GetNumVerticesTotal();
	if (num_channels) {
		topology_data.uv_count = refiner->GetNumFVarValuesTotal(Channels::UV);
	}

	return refiner;
}
void Subdivider::_create_subdivision_vertices(Far::TopologyRefiner *refiner, const int p_level, const bool face_varying_data) {
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

	if (face_varying_data) { //could be handled in the same loop as vertices, but then not usable with just updating vertices
		topology_data.uv_array.resize(topology_data.uv_count);
		VertexUV *src_uv = (VertexUV *)topology_data.uv_array.ptr();
		//face varying data like uv's, weights etc.
		for (int level = 0; level < p_level; ++level) {
			VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(Channels::UV);
			primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, Channels::UV);
			src_uv = dst_uv;
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
	bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;

	topology_data = TopologyData(p_arrays, p_format, _get_vertices_per_face_count());
	if (p_level == 0) {
		return;
	}

	const int num_channels = use_uv; //TODO: maybe add channel for tex uv 2
	const bool face_varying_data = num_channels > 0;

	Far::TopologyRefiner *refiner = _create_topology_refiner(p_level, num_channels);
	ERR_FAIL_COND_MSG(!refiner, "Refiner couldn't be created, numVertsPerFace array likely lost.");
	_create_subdivision_vertices(refiner, p_level, face_varying_data);
	_create_subdivision_faces(refiner, p_level, face_varying_data);
	//free memory
	delete refiner;

	if (calculate_normals) {
		topology_data.normal_array = _calculate_smooth_normals(topology_data.vertex_array, topology_data.index_array);
	}
}

//TODO: virtual calls are not implemented yet in godot cpp (I think, it wasnt 2 weeks ago and didn't see any commit)
OpenSubdiv::Sdc::SchemeType Subdivider::_get_refiner_type() const {
	return Sdc::SchemeType::SCHEME_CATMARK;
}

void Subdivider::_create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
		const int32_t p_level, const bool face_varying_data) {
	PackedInt32Array index_array;
	PackedInt32Array uv_index_array;

	Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
	int face_count_out = last_level.GetNumFaces();
	int uv_index_offset = face_varying_data ? topology_data.uv_count - last_level.GetNumFVarValues(Channels::UV) : -1;
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

		if (face_varying_data) {
			Far::ConstIndexArray face_uvs = last_level.GetFaceFVarValues(face_index, Channels::UV);
			for (int face_vert_index = 0; face_vert_index < topology_data.vertex_count_per_face; face_vert_index++) {
				uv_index_array.push_back(uv_index_offset + face_uvs[face_vert_index]);
			}
		}
	}
	topology_data.index_array = index_array;
	if (face_varying_data) {
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
	ClassDB::bind_method("get_subdivided_arrays", &Subdivider::get_subdivided_arrays);
	ClassDB::bind_method("get_subdivided_topology_arrays", &Subdivider::get_subdivided_topology_arrays);
}

Subdivider::Subdivider() {
}
Subdivider::~Subdivider() {
}