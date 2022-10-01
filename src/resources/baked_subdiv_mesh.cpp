#include "baked_subdiv_mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/subdivision_mesh.hpp"
#include "subdivision/subdivision_server.hpp"

int64_t BakedSubdivMesh::_get_surface_count() const {
	ERR_FAIL_COND_V(data_mesh.is_null(), 0);
	return data_mesh->get_surface_count();
}

int64_t BakedSubdivMesh::_surface_get_array_len(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), 0);
	const PackedInt32Array &index_array = RenderingServer::get_singleton()->mesh_surface_get_arrays(subdiv_mesh->get_rid(), index)[Mesh::ARRAY_INDEX];
	return index_array.size();
}
int64_t BakedSubdivMesh::_surface_get_array_index_len(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), 0);
	const PackedInt32Array &index_array = RenderingServer::get_singleton()->mesh_surface_get_arrays(subdiv_mesh->get_rid(), index)[Mesh::ARRAY_INDEX];
	return index_array.size();
}

Array BakedSubdivMesh::_surface_get_arrays(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), Array());
	return RenderingServer::get_singleton()->mesh_surface_get_arrays(subdiv_mesh->get_rid(), index);
}
TypedArray<Array> BakedSubdivMesh::_surface_get_blend_shape_arrays(int64_t index) const {
	return RenderingServer::get_singleton()->mesh_surface_get_blend_shape_arrays(subdiv_mesh->get_rid(), index);
}
Dictionary BakedSubdivMesh::_surface_get_lods(int64_t index) const {
	//TODO:
	return Dictionary();
}

int64_t BakedSubdivMesh::_surface_get_format(int64_t index) const {
	ERR_FAIL_COND_V(data_mesh.is_null(), 0);
	ERR_FAIL_INDEX_V(index, get_surface_count(), 0);
	return data_mesh->surface_get_format(index);
}

int64_t BakedSubdivMesh::_surface_get_primitive_type(int64_t index) const {
	return Mesh::PRIMITIVE_TRIANGLES;
}

void BakedSubdivMesh::_surface_set_material(int64_t index, const Ref<Material> &material) {
	ERR_FAIL_COND(data_mesh.is_null());
	ERR_FAIL_INDEX(index, get_surface_count());
	data_mesh->surface_set_material(index, material);
}
Ref<Material> BakedSubdivMesh::_surface_get_material(int64_t index) const {
	ERR_FAIL_COND_V(data_mesh.is_null(), Ref<Material>());
	ERR_FAIL_INDEX_V(index, get_surface_count(), Ref<Material>());
	return data_mesh->surface_get_material(index);
}

int64_t BakedSubdivMesh::_get_blend_shape_count() const {
	return 0; //TODO: currently returns 0 to avoid issues, would need to run Subdividers with blend shape data array and index array of mesh
}
StringName BakedSubdivMesh::_get_blend_shape_name(int64_t index) const {
	ERR_FAIL_COND_V(data_mesh.is_null(), StringName());
	ERR_FAIL_INDEX_V(index, data_mesh->get_blend_shape_count(), StringName());

	return data_mesh->get_blend_shape_name(index);
}
void BakedSubdivMesh::_set_blend_shape_name(int64_t index, const StringName &p_name) {
	ERR_FAIL_COND(data_mesh.is_null());
	ERR_FAIL_INDEX(index, data_mesh->get_blend_shape_count());
	data_mesh->set_blend_shape_name(index, p_name);
}

//only use after adding blend shape to surface
void BakedSubdivMesh::_add_blend_shape_name(const StringName &p_name) {
	ERR_FAIL_COND(data_mesh.is_null());
	data_mesh->add_blend_shape_name(p_name);
}

AABB BakedSubdivMesh::_get_aabb() const {
	//TODO:
	return AABB();
}

void BakedSubdivMesh::set_data_mesh(Ref<TopologyDataMesh> p_data_mesh) {
	data_mesh = p_data_mesh;
	_update_subdiv();
}
Ref<TopologyDataMesh> BakedSubdivMesh::get_data_mesh() const {
	return data_mesh;
}

void BakedSubdivMesh::set_subdiv_level(int p_level) {
	subdiv_level = p_level;
	_update_subdiv();
}
int BakedSubdivMesh::get_subdiv_level() const {
	return subdiv_level;
}

void BakedSubdivMesh::_update_subdiv() {
	if (data_mesh.is_null() && subdiv_mesh) {
		subdiv_mesh->clear();
		return;
	}
	if (!subdiv_mesh) {
		SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
		ERR_FAIL_COND(!subdivision_server);
		subdiv_mesh = Object::cast_to<SubdivisionMesh>(subdivision_server->create_subdivision_mesh_with_rid(data_mesh, subdiv_level, subdiv_mesh_rid));
	} else {
		subdiv_mesh->_update_subdivision(data_mesh, subdiv_level, Vector<Array>());
	}
	emit_changed();
}

RID BakedSubdivMesh::get_rid() const {
	return subdiv_mesh_rid;
}

BakedSubdivMesh::BakedSubdivMesh() {
	subdiv_mesh = NULL;
	subdiv_mesh_rid = RenderingServer::get_singleton()->mesh_create();
}

BakedSubdivMesh::~BakedSubdivMesh() {
	if (subdiv_mesh) {
		SubdivisionServer::get_singleton()->destroy_subdivision_mesh(subdiv_mesh);
	} else {
		RenderingServer::get_singleton()->free_rid(subdiv_mesh_rid);
	}
	subdiv_mesh = NULL;
}

void BakedSubdivMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_data_mesh", "data_mesh"), &BakedSubdivMesh::set_data_mesh);
	ClassDB::bind_method(D_METHOD("get_data_mesh"), &BakedSubdivMesh::get_data_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data_mesh", PROPERTY_HINT_RESOURCE_TYPE, "TopologyDataMesh"), "set_data_mesh", "get_data_mesh");
	ClassDB::bind_method(D_METHOD("set_subdiv_level", "subdiv_level"), &BakedSubdivMesh::set_subdiv_level);
	ClassDB::bind_method(D_METHOD("get_subdiv_level"), &BakedSubdivMesh::get_subdiv_level);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdiv_level", PROPERTY_HINT_RANGE, "0,6"), "set_subdiv_level", "get_subdiv_level");

	//virtuals of mesh
	BakedSubdivMesh::register_virtuals<Mesh>();
}