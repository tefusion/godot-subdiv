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
			Ref<QuadSubdivider> subdivider;
			subdivider.instantiate();
			return subdivider->get_subdivided_arrays(p_arrays, p_level, p_format, calculate_normals);
		}

		case TopologyDataMesh::TRIANGLE: {
			Ref<TriangleSubdivider> subdivider;
			subdivider.instantiate();
			return subdivider->get_subdivided_arrays(p_arrays, p_level, p_format, calculate_normals);
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
	subdiv_mesh.clear_surfaces();
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

		Ref<Material> material = p_mesh->surface_get_material(surface_index);
		subdiv_mesh.add_surface(subdiv_triangle_arrays, Dictionary(), material, "", surface_format);

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

	//TODO: also update normals
	// currently normal generation too slow to actually update
	Array subdiv_triangle_arrays = _get_subdivided_arrays(v_arrays, p_level, surface_format, false, topology_type);

	subdiv_mesh.update_surface_vertices(p_surface, subdiv_triangle_arrays);
}

void SubdivisionMesh::clear() {
	subdiv_mesh.clear_surfaces();
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
	return subdiv_mesh.get_rid();
}

void SubdivisionMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_rid"), &SubdivisionMesh::get_rid);
	ClassDB::bind_method(D_METHOD("update_subdivision"), &SubdivisionMesh::update_subdivision);
}

SubdivisionMesh::SubdivisionMesh() :
		subdiv_mesh(LocalMesh()) {
	current_level = 0;
}

SubdivisionMesh::~SubdivisionMesh() {
}
