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

#include "quad_subdivider.hpp"
#include "triangle_subdivider.hpp"

Array SubdivisionMesh::_get_subdivided_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals, TopologyDataMesh::TopologyType topology_type) {
	switch (topology_type) {
		case TopologyDataMesh::QUAD: {
			QuadSubdivider subdivider;
			return subdivider.get_subdivided_arrays(p_arrays, p_level, p_format, calculate_normals);
		}

		case TopologyDataMesh::TRIANGLE: {
			TriangleSubdivider subdivider;
			return subdivider.get_subdivided_arrays(p_arrays, p_level, p_format, calculate_normals);
		}

		default:
			return Array();
	}
}

void SubdivisionMesh::update_subdivision(Ref<TopologyDataMesh> p_mesh, int32_t p_level) {
	_update_subdivision(p_mesh, p_level, Vector<Array>()); //TODO: maybe split functions up
}
//just leave cached data arrays empty if you want to use p_mesh arrays
void SubdivisionMesh::_update_subdivision(Ref<TopologyDataMesh> p_mesh, int32_t p_level, const Vector<Array> &cached_data_arrays) {
	RenderingServer::get_singleton()->mesh_clear(subdiv_mesh);
	subdiv_vertex_count.clear();
	subdiv_index_count.clear();

	ERR_FAIL_COND(p_mesh.is_null());
	ERR_FAIL_COND(p_level < 0);
	source_mesh = p_mesh->get_rid();
	RenderingServer *rendering_server = RenderingServer::get_singleton();
	int32_t surface_count = p_mesh->get_surface_count();

	for (int32_t surface_index = 0; surface_index < surface_count; ++surface_index) {
		int32_t surface_format = p_mesh->surface_get_format(surface_index); //prepare format
		surface_format &= ~Mesh::ARRAY_FORMAT_BONES; //might cause issues on rendering server so erase bits
		surface_format &= ~Mesh::ARRAY_FORMAT_WEIGHTS;

		Array v_arrays = cached_data_arrays.size() ? cached_data_arrays[surface_index]
												   : p_mesh->surface_get_arrays(surface_index);

		Array subdiv_triangle_arrays = _get_subdivided_arrays(v_arrays, p_level, surface_format, true, p_mesh->surface_get_topology_type(surface_index));

		rendering_server->mesh_add_surface_from_arrays(subdiv_mesh, RenderingServer::PRIMITIVE_TRIANGLES, subdiv_triangle_arrays, Array(), Dictionary(), surface_format);

		Ref<Material> material = p_mesh->surface_get_material(surface_index);
		rendering_server->mesh_surface_set_material(subdiv_mesh, surface_index, material.is_null() ? RID() : material->get_rid());
		current_level = p_level;
	}
}

void SubdivisionMesh::update_subdivision_vertices(int p_surface, const PackedVector3Array &new_vertex_array,
		const PackedInt32Array &index_array, TopologyDataMesh::TopologyType topology_type) {
	int p_level = current_level;
	ERR_FAIL_COND(p_level < 0);

	int32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format &= Mesh::ARRAY_FORMAT_INDEX;

	Array v_arrays;
	v_arrays.resize(TopologyDataMesh::ARRAY_MAX);
	v_arrays[TopologyDataMesh::ARRAY_VERTEX] = new_vertex_array;
	v_arrays[TopologyDataMesh::ARRAY_INDEX] = index_array;
	QuadSubdivider subdivider;

	//TODO: also update normals
	// for putting it into an int look in immediate mesh (just shift and clamp each value to fit into 30 bits total)
	// currently normal generation too slow to actually update
	Array subdiv_triangle_arrays = _get_subdivided_arrays(v_arrays, p_level, surface_format, false, topology_type);

	const PackedInt32Array &index_array_out = subdiv_triangle_arrays[Mesh::ARRAY_INDEX];
	const PackedVector3Array &vertex_array_out = subdiv_triangle_arrays[Mesh::ARRAY_VERTEX]; //now that subdivider class used this is the already triangulated array

	// update vertices
	//Vector<uint8_t> vertex_data; // Vertex, Normal, Tangent (change with skinning, blendshape).
	PackedByteArray vertex_buffer = RenderingServer::get_singleton()->mesh_get_surface(subdiv_mesh, p_surface)["vertex_data"];
	uint8_t *vertex_write_buffer = vertex_buffer.ptrw();

	uint32_t vertex_stride = sizeof(float) * 3; //vector3 size
	int normal_offset = vertex_stride;
	int tangent_offset = vertex_stride + sizeof(int);

	if (vertex_buffer.size() / vertex_stride != vertex_array_out.size() && vertex_buffer.size() % vertex_array_out.size() == 0) {
		vertex_stride = vertex_buffer.size() / vertex_array_out.size(); //if not already equal likely also contains normals and/or tangents
		//if the division has a remainder data corrupted, will then autofail in condition below
	}

	ERR_FAIL_COND(vertex_buffer.size() / vertex_stride != vertex_array_out.size());

	bool has_normals = vertex_stride >= sizeof(float) * 4;
	bool has_tangents = vertex_stride >= sizeof(float) * 5; //TODO: tangents don't get returned because uv's not interpolated
	// for skinning

	for (int vertex_index = 0; vertex_index < vertex_array_out.size(); vertex_index++) {
		memcpy(&vertex_write_buffer[vertex_index * vertex_stride], &vertex_array_out[vertex_index], sizeof(float) * 3);
	}

	RenderingServer::get_singleton()->mesh_surface_update_vertex_region(subdiv_mesh, p_surface, 0, vertex_buffer);
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

RID SubdivisionMesh::get_rid() const {
	return subdiv_mesh;
}

void SubdivisionMesh::set_rid(RID p_rid) {
	if (subdiv_mesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(subdiv_mesh);
	}
	subdiv_mesh = p_rid;
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
