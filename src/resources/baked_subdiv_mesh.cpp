#include "baked_subdiv_mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/subdivision_baker.hpp"
#include "subdivision/subdivision_mesh.hpp"
#include "subdivision/subdivision_server.hpp"

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
	if (data_mesh.is_valid()) {
		SubdivisionBaker baker;
		_clear();

		//add blendshapes
		if (data_mesh->get_blend_shape_count() != get_blend_shape_count()) {
			clear_blend_shapes();
			for (int blend_shape_idx = 0; blend_shape_idx < data_mesh->get_blend_shape_count(); blend_shape_idx++) {
				add_blend_shape(data_mesh->get_blend_shape_name(blend_shape_idx));
			}
		}

		for (int surface_index = 0; surface_index < data_mesh->get_surface_count(); surface_index++) {
			const Array &source_arrays = data_mesh->surface_get_arrays(surface_index);
			int64_t p_format = data_mesh->surface_get_format(surface_index);
			TopologyDataMesh::TopologyType topology_type = data_mesh->surface_get_topology_type(surface_index);
			const Array &triangle_arrays = baker.get_baked_arrays(source_arrays,
					subdiv_level, p_format, topology_type);

			//bake blendshapes
			TypedArray<Array> baked_blend_shape_arrays;
			if (data_mesh->get_blend_shape_count() > 0) {
				const Array &blend_shape_arrays = data_mesh->surface_get_blend_shape_arrays(surface_index);
				baked_blend_shape_arrays = baker.get_baked_blend_shape_arrays(source_arrays, blend_shape_arrays,
						subdiv_level, p_format, topology_type);
			}
			add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, triangle_arrays, baked_blend_shape_arrays, Dictionary(), 0);
			surface_set_name(surface_index, data_mesh->surface_get_name(surface_index));
			surface_set_material(surface_index, data_mesh->surface_get_material(surface_index));
		}
	}
	emit_changed();
}

void BakedSubdivMesh::_clear() {
	clear_surfaces();
}

bool BakedSubdivMesh::_set(const StringName &p_name, const Variant &p_value) {
	String s_name = p_name;
	if (s_name == "subdiv_level") {
		set_subdiv_level(p_value); //updates subdiv
		return true;
	} else if (s_name.begins_with("_surfaces") || s_name.begins_with("_blend_shape_names")) {
		return true;
	}
	return false;
}
bool BakedSubdivMesh::_get(const StringName &p_name, Variant &r_ret) {
	String s_name = p_name;
	if (s_name == "subdiv_level") {
		r_ret = get_subdiv_level();
		return true;
	} else if (s_name.begins_with("_surfaces") || s_name.begins_with("_blend_shape_names")) {
		return true;
	}

	return false;
}

BakedSubdivMesh::BakedSubdivMesh() {
}

BakedSubdivMesh::~BakedSubdivMesh() {
}

void BakedSubdivMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("sedata_mesh", "data_mesh"), &BakedSubdivMesh::set_data_mesh);
	ClassDB::bind_method(D_METHOD("gedata_mesh"), &BakedSubdivMesh::get_data_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data_mesh", PROPERTY_HINT_RESOURCE_TYPE, "TopologyDataMesh"), "sedata_mesh", "gedata_mesh");
	ClassDB::bind_method(D_METHOD("set_subdiv_level", "subdiv_level"), &BakedSubdivMesh::set_subdiv_level);
	ClassDB::bind_method(D_METHOD("get_subdiv_level"), &BakedSubdivMesh::get_subdiv_level);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdiv_level", PROPERTY_HINT_RANGE, "0,6"), "set_subdiv_level", "get_subdiv_level");

	//virtuals of mesh
	BakedSubdivMesh::register_virtuals<Mesh>();
}