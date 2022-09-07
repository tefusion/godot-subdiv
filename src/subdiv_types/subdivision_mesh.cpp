#include "subdivision_mesh.hpp"

#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/core/class_db.hpp"

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include "godot_cpp/classes/mesh_data_tool.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/builtin_types.hpp"

#include "far/primvarRefiner.h"
#include "far/topologyDescriptor.h"

//debug
// #include <chrono>
// using namespace std::chrono;

using namespace OpenSubdiv;
typedef Far::TopologyDescriptor Descriptor;

SubdivisionMesh::SubdivData::SubdivData(const Array &p_mesh_arrays) {
	vertex_array = p_mesh_arrays[SubdivDataMesh::ARRAY_VERTEX];
	normal_array = p_mesh_arrays[SubdivDataMesh::ARRAY_NORMAL];
	index_array = p_mesh_arrays[SubdivDataMesh::ARRAY_INDEX];
	uv_array = p_mesh_arrays[SubdivDataMesh::ARRAY_TEX_UV];
	uv_index_array = p_mesh_arrays[SubdivDataMesh::ARRAY_UV_INDEX];
	bones_array = p_mesh_arrays[SubdivDataMesh::ARRAY_BONES];
	weights_array = p_mesh_arrays[SubdivDataMesh::ARRAY_WEIGHTS];

	index_count = index_array.size();
	face_count = index_array.size() / 4;
	vertex_count = vertex_array.size();
	uv_count = uv_index_array.size();
	bone_count = bones_array.size();
	weight_count = weights_array.size();
}

SubdivisionMesh::SubdivData::SubdivData(const PackedVector3Array &p_vertex_array, const PackedInt32Array &p_index_array) {
	vertex_array = p_vertex_array;
	index_array = p_index_array;
	index_count = index_array.size();
	face_count = index_array.size() / 4;
	vertex_count = vertex_array.size();
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

Descriptor SubdivisionMesh::_create_topology_descriptor(
		const SubdivData &subdiv, const int num_channels) {
	Descriptor desc;
	desc.numVertices = subdiv.vertex_count;
	desc.numFaces = subdiv.face_count;
	desc.vertIndicesPerFace = subdiv.index_array.ptr();

	return desc;
}

Far::TopologyRefiner *SubdivisionMesh::_create_topology_refiner(
		SubdivData *subdiv, const int32_t p_level, const int num_channels) {
	//create descriptor,
	//FIXME: some values have to be declared within function here, memnew also didn't work
	Vector<int> subdiv_face_vertex_count;
	subdiv_face_vertex_count.resize(subdiv->face_count);
	subdiv_face_vertex_count.fill(4);
	Descriptor desc = _create_topology_descriptor(*subdiv, num_channels);
	desc.numVertsPerFace = subdiv_face_vertex_count.ptr();
	Descriptor::FVarChannel channels[num_channels];
	if (num_channels > 0) {
		channels[Channels::UV].numValues = subdiv->uv_count;
		channels[Channels::UV].valueIndices = subdiv->uv_index_array.ptr();
		desc.numFVarChannels = num_channels;
		desc.fvarChannels = channels;
	}

	Sdc::SchemeType type = Sdc::SCHEME_CATMARK;
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

	Far::TopologyRefiner *refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
	//ERR_FAIL_COND(!refiner);
	ERR_FAIL_COND_V(!refiner, nullptr);

	Far::TopologyRefiner::UniformOptions refine_options(p_level);
	refine_options.fullTopologyInLastLevel = true;
	refiner->RefineUniform(refine_options);

	subdiv->vertex_count = refiner->GetNumVerticesTotal();
	if (num_channels) {
		subdiv->uv_count = refiner->GetNumFVarValuesTotal(Channels::UV);
	}

	return refiner;
}

void SubdivisionMesh::_create_subdivision_vertices(
		SubdivData *subdiv, Far::TopologyRefiner *refiner, const int p_level, const bool face_varying_data) {
	subdiv->vertex_array.resize(subdiv->vertex_count);
	// Interpolate vertex primvar data
	Far::PrimvarRefiner primvar_refiner(*refiner);
	//vertices
	Vertex *src = (Vertex *)subdiv->vertex_array.ptr();
	for (int level = 0; level < p_level; ++level) {
		Vertex *dst = src + refiner->GetLevel(level).GetNumVertices();
		primvar_refiner.Interpolate(level + 1, src, dst);
		src = dst;
	}

	if (face_varying_data) { //could be handled in the same loop as vertices, but then not usable with just updating vertices
		subdiv->uv_array.resize(subdiv->uv_count);
		VertexUV *src_uv = (VertexUV *)subdiv->uv_array.ptr();
		//face varying data like uv's, weights etc.
		for (int level = 0; level < p_level; ++level) {
			VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(Channels::UV);
			primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, Channels::UV);
			src_uv = dst_uv;
		}
	}
}

void SubdivisionMesh::_create_subdivision_faces(const SubdivData &subdiv, Far::TopologyRefiner *refiner, const int p_level,
		const bool face_varying_data, Array &subdiv_quad_arrays) {
	PackedInt32Array index_array;
	PackedInt32Array uv_index_array;

	Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
	int face_count_out = last_level.GetNumFaces();
	int uv_index_offset = face_varying_data ? subdiv.uv_count - last_level.GetNumFVarValues(Channels::UV) : -1;
	int vertex_index_offset = subdiv.vertex_count - last_level.GetNumVertices();
	for (int face_index = 0; face_index < face_count_out; ++face_index) {
		int parent_face_index = last_level.GetFaceParentFace(face_index);
		for (int level_index = p_level - 1; level_index > 0; --level_index) {
			Far::TopologyLevel const &prev_level = refiner->GetLevel(level_index);
			parent_face_index = prev_level.GetFaceParentFace(parent_face_index);
		}

		Far::ConstIndexArray face_vertices = last_level.GetFaceVertices(face_index);

		ERR_FAIL_COND_MSG(face_vertices.size() != 4, "Faces need to be quads.");
		for (int face_vert_index = 0; face_vert_index < 4; face_vert_index++) {
			index_array.push_back(vertex_index_offset + face_vertices[face_vert_index]);
		}

		if (face_varying_data) {
			Far::ConstIndexArray face_uvs = last_level.GetFaceFVarValues(face_index, Channels::UV);
			for (int face_vert_index = 0; face_vert_index < 4; face_vert_index++) {
				uv_index_array.push_back(uv_index_offset + face_uvs[face_vert_index]);
			}
		}
	}
	subdiv_quad_arrays[SubdivDataMesh::ARRAY_INDEX] = index_array;
	subdiv_quad_arrays[SubdivDataMesh::ARRAY_VERTEX] = subdiv.vertex_array;
	if (face_varying_data) {
		subdiv_quad_arrays[SubdivDataMesh::ARRAY_UV_INDEX] = uv_index_array;
		subdiv_quad_arrays[SubdivDataMesh::ARRAY_TEX_UV] = subdiv.uv_array;
	}
}
void SubdivisionMesh::update_subdivision(Ref<SubdivDataMesh> p_mesh, int32_t p_level) {
	_update_subdivision(p_mesh, p_level, Vector<Array>()); //TODO: maybe split functions up
}
//just leave cached data arrays empty if you want to use p_mesh arrays
void SubdivisionMesh::_update_subdivision(Ref<SubdivDataMesh> p_mesh, int32_t p_level, const Vector<Array> &cached_data_arrays) {
	//auto start = high_resolution_clock::now(); //time measuring code
	RenderingServer::get_singleton()->mesh_clear(subdiv_mesh);
	subdiv_vertex_count.clear();
	subdiv_index_count.clear();

	ERR_FAIL_COND(p_mesh.is_null());
	ERR_FAIL_COND(p_level < 0);
	source_mesh = p_mesh->get_rid();
	RenderingServer *rendering_server = RenderingServer::get_singleton();
	uint32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format |= Mesh::ARRAY_FORMAT_TEX_UV;
	surface_format |= Mesh::ARRAY_FORMAT_INDEX;
	surface_format |= Mesh::ARRAY_FORMAT_NORMAL;
	// surface_format &= ~Mesh::ARRAY_COMPRESS_FLAGS_BASE;
	// surface_format |= Mesh::ARRAY_FORMAT_TANGENT;
	// surface_format |= Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE;

	if (p_level == 0) {
		for (int32_t surface_index = 0; surface_index < p_mesh->get_surface_count(); ++surface_index) {
			Array triangle_arrays = cached_data_arrays.size() ? p_mesh->generate_trimesh_arrays_from_quad_arrays(cached_data_arrays[surface_index])
															  : p_mesh->generate_trimesh_arrays(surface_index);
			rendering_server->mesh_add_surface_from_arrays(subdiv_mesh, RenderingServer::PRIMITIVE_TRIANGLES, triangle_arrays, Array(), Dictionary(), surface_format);
			Ref<Material> material = p_mesh->_surface_get_material(surface_index); //TODO: currently calls function directly cause virtual surface_get_material still gives corrupt data?
			rendering_server->mesh_surface_set_material(subdiv_mesh, surface_index, material.is_null() ? RID() : material->get_rid());

			const PackedVector3Array &vertex_array = triangle_arrays[Mesh::ARRAY_VERTEX];
			const PackedInt32Array &index_array = triangle_arrays[Mesh::ARRAY_INDEX];
			subdiv_vertex_count.append(vertex_array.size());
			subdiv_index_count.append(index_array.size());
		}
		current_level = p_level;
		return;
	}

	int32_t surface_count = p_mesh->get_surface_count();

	for (int32_t surface_index = 0; surface_index < surface_count; ++surface_index) {
		Array v_arrays = cached_data_arrays.size() ? cached_data_arrays[surface_index]
												   : p_mesh->surface_get_data_arrays(surface_index);
		SubdivData subdiv = SubdivData(v_arrays);

		const int num_channels = 1;

		Far::TopologyRefiner *refiner;

		refiner = _create_topology_refiner(&subdiv, p_level, num_channels);
		ERR_FAIL_COND_MSG(!refiner, "Refiner couldn't be created, numVertsPerFace array likely lost.");
		_create_subdivision_vertices(&subdiv, refiner, p_level, true);

		Array subdiv_quad_arrays;
		subdiv_quad_arrays.resize(SubdivDataMesh::ARRAY_MAX);
		_create_subdivision_faces(subdiv, refiner, p_level, true, subdiv_quad_arrays);
		subdiv.normal_array = _calculate_smooth_normals(subdiv.vertex_array, subdiv_quad_arrays[SubdivDataMesh::ARRAY_INDEX]);
		const PackedInt32Array &index_array_out = subdiv_quad_arrays[SubdivDataMesh::ARRAY_INDEX];
		const PackedInt32Array &uv_index_array_out = subdiv_quad_arrays[SubdivDataMesh::ARRAY_UV_INDEX];

		//free memory
		delete refiner;

		Array subdiv_triangle_arrays;
		subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);
		PackedVector2Array uv_array;
		PackedVector3Array vertex_array;
		PackedInt32Array index_array;
		PackedVector3Array normal_array;

		for (int quad_index = 0; quad_index < index_array_out.size(); quad_index += 4) {
			//add vertices part of quad

			//after for loop unshared0 vertex will be at the positon quad_index in the new vertex_array in the SurfaceTool
			for (int single_quad_index = quad_index; single_quad_index < quad_index + 4; single_quad_index++) {
				uv_array.append(subdiv.uv_array[uv_index_array_out[single_quad_index]]);
				vertex_array.append(subdiv.vertex_array[index_array_out[single_quad_index]]);
				normal_array.append(subdiv.normal_array[index_array_out[single_quad_index]]);
			} //unshared0, shared0, unshared1, shared1

			//add triangle 1 with unshared0
			index_array.append(quad_index);
			index_array.append(quad_index + 1);
			index_array.append(quad_index + 3);

			//add triangle 2 with unshared1
			index_array.append(quad_index + 1);
			index_array.append(quad_index + 2);
			index_array.append(quad_index + 3);
		}

		subdiv_triangle_arrays[Mesh::ARRAY_VERTEX] = vertex_array;
		subdiv_triangle_arrays[Mesh::ARRAY_INDEX] = index_array;
		subdiv_triangle_arrays[Mesh::ARRAY_TEX_UV] = uv_array;
		subdiv_triangle_arrays[Mesh::ARRAY_NORMAL] = normal_array;

		subdiv_vertex_count.append(vertex_array.size());
		subdiv_index_count.append(index_array.size());

		rendering_server->mesh_add_surface_from_arrays(subdiv_mesh, RenderingServer::PRIMITIVE_TRIANGLES, subdiv_triangle_arrays, Array(), Dictionary(), surface_format);

		Ref<Material> material = p_mesh->_surface_get_material(surface_index);
		rendering_server->mesh_surface_set_material(subdiv_mesh, surface_index, material.is_null() ? RID() : material->get_rid());
		current_level = p_level;
	}
}

void SubdivisionMesh::update_subdivision_vertices(int p_surface, const PackedVector3Array &new_vertex_array, const PackedInt32Array &index_array) {
	int p_level = current_level;
	if (p_level < 0) {
		return;
	}
	RenderingServer *rendering_server = RenderingServer::get_singleton();

	if (p_level == 0) {
		//Vector<uint8_t> vertex_data; // Vertex, Normal, Tangent (change with skinning, blendshape).
		PackedByteArray vertex_buffer = rendering_server->mesh_get_surface(subdiv_mesh, p_surface)["vertex_data"]; //also contains vertex_count, etc. maybe do check if same
		uint8_t *vertex_write_buffer = vertex_buffer.ptrw();

		uint32_t vertex_stride = sizeof(float) * 3; //vector3 size
		int normal_offset = vertex_stride;
		vertex_stride += sizeof(int); //normal

		for (int i = 0; i < index_array.size(); i++) {
			memcpy(&vertex_write_buffer[i * vertex_stride], &new_vertex_array[index_array[i]], sizeof(float) * 3);
		}

		RenderingServer::get_singleton()->mesh_surface_update_vertex_region(subdiv_mesh, p_surface, 0, vertex_buffer);
		return;
	}

	SubdivData subdiv = SubdivData(new_vertex_array, index_array);
	const int num_channels = 0; //when updating vertices, channels not used
	Far::TopologyRefiner *refiner;
	refiner = _create_topology_refiner(&subdiv, current_level, num_channels);
	ERR_FAIL_COND_MSG(!refiner, "Refiner couldn't be created, numVertsPerFace array likely lost.");
	_create_subdivision_vertices(&subdiv, refiner, p_level, false);
	Array subdiv_quad_arrays;
	subdiv_quad_arrays.resize(SubdivDataMesh::ARRAY_MAX);
	_create_subdivision_faces(subdiv, refiner, p_level, false, subdiv_quad_arrays);

	//TODO: calculate normal value you can put into the buffer (size only of int)
	//subdiv.normal_array = _calculate_smooth_normals(subdiv.vertex_array, subdiv_quad_arrays[SubdivDataMesh::ARRAY_INDEX]);

	//free memory
	delete refiner;

	const PackedInt32Array &index_array_out = subdiv_quad_arrays[SubdivDataMesh::ARRAY_INDEX];

	// update vertices
	PackedByteArray vertex_buffer = rendering_server->mesh_get_surface(subdiv_mesh, p_surface)["vertex_data"];
	uint8_t *vertex_write_buffer = vertex_buffer.ptrw();

	uint32_t vertex_stride = sizeof(float) * 3; //vector3 size
	int normal_offset = vertex_stride;
	vertex_stride += sizeof(int); //normal

	for (int quad_index = 0; quad_index < index_array_out.size(); quad_index++) {
		memcpy(&vertex_write_buffer[quad_index * vertex_stride], &subdiv.vertex_array[index_array_out[quad_index]], sizeof(float) * 3);
	}

	RenderingServer::get_singleton()->mesh_surface_update_vertex_region(subdiv_mesh, p_surface, 0, vertex_buffer);
}

//only for quads, returns normal_array
PackedVector3Array SubdivisionMesh::_calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array) {
	PackedVector3Array normals;
	normals.resize(quad_vertex_array.size());
	for (int f = 0; f < quad_index_array.size(); f += 4) {
		// // We will use the first three verts to calculate a normal
		const Vector3 &p_point1 = quad_vertex_array[quad_index_array[f]];
		const Vector3 &p_point2 = quad_vertex_array[quad_index_array[f + 1]];
		const Vector3 &p_point3 = quad_vertex_array[quad_index_array[f + 2]];
		Vector3 normal_calculated = (p_point1 - p_point3).cross(p_point1 - p_point2);
		normal_calculated.normalize();
		for (int n_pos = f; n_pos < f + 4; n_pos++) {
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

void SubdivisionMesh::clear() {
	RenderingServer::get_singleton()->mesh_clear(subdiv_mesh);
	subdiv_vertex_count.clear();
	subdiv_index_count.clear();
}

int64_t SubdivisionMesh::surface_get_vertex_array_size(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, subdiv_vertex_count.size(), 0);
	return subdiv_vertex_count[p_surface];
}

int64_t SubdivisionMesh::surface_get_index_array_size(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, subdiv_index_count.size(), 0);
	return subdiv_index_count[p_surface];
}

void SubdivisionMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_rid"), &SubdivisionMesh::get_rid);
	ClassDB::bind_method(D_METHOD("update_subdivision"), &SubdivisionMesh::update_subdivision);
}

SubdivisionMesh::SubdivisionMesh() {
	current_level = 0;
	subdiv_mesh = RenderingServer::get_singleton()->mesh_create();
}

SubdivisionMesh::~SubdivisionMesh() {
	if (subdiv_mesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(subdiv_mesh);
	}

	subdiv_mesh = RID();
}
