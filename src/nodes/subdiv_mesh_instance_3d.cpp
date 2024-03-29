/*
Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.
Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "subdiv_mesh_instance_3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"

#include "godot_cpp/classes/surface_tool.hpp"

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "subdivision/subdivision_server.hpp"

#include <string>

//editor functions
void SubdivMeshInstance3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_mesh_changed();
			_resolve_skeleton_path();
		} break;
	}
}

void SubdivMeshInstance3D::_get_property_list(List<PropertyInfo> *p_list) const {
	List<String> blend_shape_name_list;
	for (const KeyValue<StringName, int> &E : blend_shape_properties) {
		blend_shape_name_list.push_back(String(E.key));
	}
	blend_shape_name_list.sort();
	for (int blend_shape_pos = 0; blend_shape_pos < blend_shape_name_list.size(); blend_shape_pos++) {
		char *blend_shape_name = new char[blend_shape_name_list[blend_shape_pos].length() + 1];
		memcpy(blend_shape_name, blend_shape_name_list[blend_shape_pos].utf8().get_data(), blend_shape_name_list[blend_shape_pos].length() + 1);
		p_list->push_back(PropertyInfo(Variant::FLOAT, blend_shape_name, PROPERTY_HINT_RANGE, "-1,1,0.00001"));
	}

	if (mesh.is_valid()) {
		for (int i = 0; i < mesh->get_surface_count(); i++) {
			std::string surface_override_name_string = "surface_material_override/" + std::to_string(i);
			const char *surface_override_name = surface_override_name_string.c_str();
			p_list->push_back(PropertyInfo(Variant::OBJECT, surface_override_name, PROPERTY_HINT_RESOURCE_TYPE, "BaseMaterial3D,ShaderMaterial", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_DEFERRED_SET_RESOURCE));
		}
	}
}

//only for blendshapes and surface overrride materials
bool SubdivMeshInstance3D::_set(const StringName &p_name, const Variant &p_value) {
	if (!get_instance().is_valid()) {
		return false;
	}
	HashMap<StringName, int>::ConstIterator found = blend_shape_properties.find(p_name);
	if (found) {
		set_blend_shape_value(found->value, p_value);
		_update_subdiv();
		return true;
	}

	if (String(p_name).begins_with("surface_material_override/")) {
		int idx = String(p_name).get_slicec('/', 1).to_int();
		if (idx >= surface_override_materials.size() || idx < 0) {
			return false;
		}
		set_surface_override_material(idx, p_value);
		return true;
	}

	return false;
}

//only for blendshapes and surface overrride materials
bool SubdivMeshInstance3D::_get(const StringName &p_name, Variant &return_value) const {
	if (!get_instance().is_valid()) {
		return false;
	}

	HashMap<StringName, int>::ConstIterator found = blend_shape_properties.find(p_name);
	if (found) {
		return_value = get_blend_shape_value(found->value);
		return true;
	}

	if (String(p_name).begins_with("surface_material_override/")) {
		int idx = String(p_name).get_slicec('/', 1).to_int();
		if (idx >= surface_override_materials.size() || idx < 0) {
			return false;
		}
		return_value = surface_override_materials[idx];
		return true;
	}
	return false;
}

//start of non editor stuff
void SubdivMeshInstance3D::set_mesh(const Ref<TopologyDataMesh> &p_mesh) {
	if (mesh.is_valid()) {
		mesh->disconnect(StringName("changed"), Callable(this, "_mesh_changed"));
	}
	mesh = p_mesh;
	if (mesh.is_valid()) {
		mesh->connect(StringName("changed"), Callable(this, "_mesh_changed"));

		_init_cached_data_array();
		if (is_inside_tree()) {
			_mesh_changed();
		}
	}
}

Ref<TopologyDataMesh> SubdivMeshInstance3D::get_mesh() const {
	return mesh;
}

void SubdivMeshInstance3D::set_skin(const Ref<Skin> &p_skin) {
	skin_internal = p_skin;
	skin = p_skin;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}
Ref<Skin> SubdivMeshInstance3D::get_skin() const {
	return skin;
}

void SubdivMeshInstance3D::set_surface_material(int p_idx, const Ref<Material> &p_material) {
	ERR_FAIL_COND(p_idx < 0);
	if (p_idx >= surface_materials.size()) {
		surface_materials.resize(p_idx + 1);
	}

	surface_materials.write[p_idx] = p_material;
}
Ref<Material> SubdivMeshInstance3D::get_surface_material(int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Ref<Material>());
	if (p_idx >= surface_materials.size()) {
		return Ref<Material>();
	}
	return surface_materials[p_idx];
}

void SubdivMeshInstance3D::set_skeleton_path(const NodePath &p_path) {
	if (is_inside_tree()) {
		Skeleton3D *skeleton = get_node<Skeleton3D>(skeleton_path);
		if (skeleton) {
			Callable callable = Callable(this, "_update_skinning");
			if (skeleton->is_connected("pose_updated", callable)) {
				skeleton->disconnect("pose_updated", callable);
			}
		}
		_resolve_skeleton_path();
	}

	skeleton_path = p_path;
}

NodePath SubdivMeshInstance3D::get_skeleton_path() const {
	return skeleton_path;
}

void SubdivMeshInstance3D::set_subdiv_level(int p_level) {
	ERR_FAIL_COND(p_level < 0);
	subdiv_level = p_level;
	if (is_inside_tree()) {
		_update_subdiv();
		_subdiv_mesh_changed();
	}
}

int32_t SubdivMeshInstance3D::get_subdiv_level() {
	return subdiv_level;
}

float SubdivMeshInstance3D::get_blend_shape_value(int p_blend_shape) const {
	ERR_FAIL_COND_V(get_mesh().is_null(), 0.0);
	ERR_FAIL_INDEX_V(p_blend_shape, (int)blend_shape_tracks.size(), 0);
	return blend_shape_tracks[p_blend_shape];
}

//need to call update subdiv manually, removed cause this gets called too often at startup
void SubdivMeshInstance3D::set_blend_shape_value(int p_blend_shape, float p_value) {
	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_tracks.size());
	float last_value = blend_shape_tracks.get(p_blend_shape);
	blend_shape_tracks.set(p_blend_shape, p_value);

	//update cached array, currently only updates vertex values
	ERR_FAIL_COND(cached_data_array.is_empty());
	if (mesh.is_valid()) {
		for (int surface_idx = 0; surface_idx < mesh->get_surface_count(); surface_idx++) {
			const Array &blend_shape_offset = mesh->surface_get_single_blend_shape_array(surface_idx, p_blend_shape);
			const Array &base_shape = mesh->surface_get_arrays(surface_idx);
			const PackedVector3Array &blend_shape_vertex_offsets = blend_shape_offset[TopologyDataMesh::ARRAY_VERTEX];
			Array &target_surface = cached_data_array.write[surface_idx];
			PackedVector3Array surface_vertex_array = target_surface[TopologyDataMesh::ARRAY_VERTEX]; //maybe reference, array makes that hard
			ERR_FAIL_COND(surface_vertex_array.size() != blend_shape_vertex_offsets.size());
			float offset = p_value - last_value;
			for (int vertex_idx = 0; vertex_idx < surface_vertex_array.size(); vertex_idx++) {
				surface_vertex_array[vertex_idx] += offset * (blend_shape_vertex_offsets[vertex_idx]);
			}
			target_surface[TopologyDataMesh::ARRAY_VERTEX] = surface_vertex_array;
		}
	}
}

void SubdivMeshInstance3D::_update_skinning() {
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	ERR_FAIL_COND(!is_visible_in_tree());
#else
	ERR_FAIL_COND(!is_visible());
#endif

	ERR_FAIL_COND(skin_ref.is_null());
	RID skeleton = skin_ref->get_skeleton();
	ERR_FAIL_COND(!skeleton.is_valid());

	RenderingServer *rendering_server = RenderingServer::get_singleton();

	// Prepare bone transforms.
	const int num_bones = rendering_server->skeleton_get_bone_count(skeleton);
	ERR_FAIL_COND(num_bones <= 0);

	Transform3D *bone_transforms = (Transform3D *)alloca(sizeof(Transform3D) * num_bones);
	for (int bone_index = 0; bone_index < num_bones; ++bone_index) {
		bone_transforms[bone_index] = rendering_server->skeleton_bone_get_transform(skeleton, bone_index);
	}

	// Apply skinning.
	int surface_count = get_mesh()->get_surface_count();

	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		BitField<Mesh::ArrayFormat> format = get_mesh()->surface_get_format(surface_index);
		ERR_CONTINUE(!(format & Mesh::ARRAY_FORMAT_BONES) || !(format & Mesh::ARRAY_FORMAT_WEIGHTS));
		bool double_bone_weights = format.has_flag(Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS);
		int weights_per_vert = double_bone_weights ? 8 : 4;

		Array mesh_arrays = _get_cached_data_array(surface_index);
		PackedVector3Array vertex_array = mesh_arrays[TopologyDataMesh::ARRAY_VERTEX];
		const PackedInt32Array &bones_array = mesh_arrays[TopologyDataMesh::ARRAY_BONES];
		const PackedFloat32Array &weights_array = mesh_arrays[TopologyDataMesh::ARRAY_WEIGHTS];

		ERR_FAIL_COND(bones_array.size() != weights_array.size() || bones_array.size() != vertex_array.size() * weights_per_vert);

		for (int vertex_index = 0; vertex_index < vertex_array.size(); ++vertex_index) {
			int offset = vertex_index * weights_per_vert;
			Vector3 origin;
			Vector3 basis_x;
			Vector3 basis_y;
			Vector3 basis_z;

			for (int bone_idx = 0; bone_idx < weights_per_vert; bone_idx++) {
				float bone_weight = weights_array[offset + bone_idx];
				int bone = bones_array[offset + bone_idx];
				origin += bone_weight * bone_transforms[bone].origin;
				basis_x += bone_weight * bone_transforms[bone].basis.get_column(0);
				basis_y += bone_weight * bone_transforms[bone].basis.get_column(1);
				basis_z += bone_weight * bone_transforms[bone].basis.get_column(2);
			}
			Transform3D transform;
			transform.origin = origin;
			transform.basis = Basis(basis_x, basis_y, basis_z);
			vertex_array[vertex_index] = transform.xform(vertex_array[vertex_index]);
		}
		_update_subdiv_mesh_vertices(surface_index, vertex_array);
	}
}

//calls subdiv mesh function to rerun subdivision with custom vertex array
void SubdivMeshInstance3D::_update_subdiv_mesh_vertices(int p_surface, const PackedVector3Array &vertex_array) {
	ERR_FAIL_COND(vertex_array.size() != get_mesh()->surface_get_length(p_surface));
	const PackedInt32Array &index_array = get_mesh()->surface_get_arrays(p_surface)[TopologyDataMesh::ARRAY_INDEX];
	subdiv_mesh->update_subdivision_vertices(p_surface, vertex_array, index_array, mesh->surface_get_topology_type(p_surface));
}

void SubdivMeshInstance3D::_resolve_skeleton_path() {
	Ref<SkinReference> new_skin_reference;
	if (!skeleton_path.is_empty()) {
		Skeleton3D *skeleton = get_node<Skeleton3D>(skeleton_path);
		if (skeleton) {
			if (skin_internal.is_null()) {
				new_skin_reference = skeleton->register_skin(skeleton->create_skin_from_rest_transforms());
				//a skin was created for us
				skin_internal = new_skin_reference->get_skin();
				notify_property_list_changed();
			} else {
				new_skin_reference = skeleton->register_skin(skin_internal);
			}
		}
	}

	skin_ref = new_skin_reference;

	if (skin_ref.is_valid()) {
		RenderingServer::get_singleton()->instance_attach_skeleton(get_instance(), skin_ref->get_skeleton());
		Skeleton3D *skeleton = get_node<Skeleton3D>(skeleton_path);
		if (skeleton) {
			Callable callable = Callable(this, "_update_skinning");
			if (!skeleton->is_connected("pose_updated", callable)) {
				skeleton->connect("pose_updated", callable);
			}
		}
	} else {
		RenderingServer::get_singleton()->instance_attach_skeleton(get_instance(), RID());
	}
}

void SubdivMeshInstance3D::_update_subdiv() {
	//this is to safe on computing power, only allow updating when actually shown/in tree
	if (!is_inside_tree()) {
		return;
	}

	if ((get_mesh().is_null() || get_mesh()->get_surface_count() == 0) && subdiv_mesh) {
		subdiv_mesh->clear();
		return;
	}

	if (!subdiv_mesh) {
		SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
		ERR_FAIL_COND(!subdivision_server);
		subdiv_mesh = Object::cast_to<SubdivisionMesh>(subdivision_server->create_subdivision_mesh(get_mesh(), subdiv_level));
		set_base(subdiv_mesh->get_rid());
	} else {
		subdiv_mesh->_update_subdivision(get_mesh(), subdiv_level, cached_data_array);
		if (skin_ref.is_valid()) { //would throw num_bones<=0 if used outside else
			_update_skinning();
		}
	}
}

//returns mesh->surface_get_arrays if cached_data_array empty
Array SubdivMeshInstance3D::_get_cached_data_array(int p_surface) const {
	if (cached_data_array.size() == 0) {
		return get_mesh()->surface_get_arrays(p_surface);
	} else {
		ERR_FAIL_INDEX_V(p_surface, cached_data_array.size(), Array());
		return cached_data_array[p_surface];
	}
}

//before mesh changed
void SubdivMeshInstance3D::_init_cached_data_array() {
	cached_data_array.clear();
	for (int surface_idx = 0; surface_idx < get_mesh()->get_surface_count(); surface_idx++) {
		cached_data_array.push_back(get_mesh()->surface_get_arrays(surface_idx));
	}
	surface_override_materials.resize(mesh->get_surface_count());
}
void SubdivMeshInstance3D::_mesh_changed() {
	if (mesh.is_null()) {
		return;
	}

	if (cached_data_array.size() != mesh->get_surface_count()) {
		_init_cached_data_array();
	}

	uint32_t initialize_bs_from = blend_shape_tracks.size();
	blend_shape_tracks.resize(mesh->get_blend_shape_count());

	for (uint32_t i = 0; i < blend_shape_tracks.size(); i++) {
		blend_shape_properties["blend_shapes/" + String(mesh->get_blend_shape_name(i))] = i;

		//set_blend_shape_value changes mesh based on previous value so set to 0 before initializing
		float tmp = blend_shape_tracks[i];
		blend_shape_tracks.write[i] = 0;
		if (i < initialize_bs_from) {
			set_blend_shape_value(i, tmp);
		} else {
			set_blend_shape_value(i, 0);
		}
	}
	_update_subdiv();
	_subdiv_mesh_changed();

	notify_property_list_changed();
	update_gizmos();
}

void SubdivMeshInstance3D::_subdiv_mesh_changed() {
	int surface_count = mesh->get_surface_count();
	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		if (surface_override_materials[surface_index].is_valid()) {
			RenderingServer::get_singleton()->instance_set_surface_override_material(get_instance(), surface_index, surface_override_materials[surface_index]->get_rid());
		}
	}
}

int SubdivMeshInstance3D::get_surface_override_material_count() const {
	return surface_override_materials.size();
}

void SubdivMeshInstance3D::set_surface_override_material(int p_surface, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_surface, surface_override_materials.size());

	surface_override_materials.write[p_surface] = p_material;

	if (subdiv_mesh) {
		if (surface_override_materials[p_surface].is_valid()) {
			RenderingServer::get_singleton()->instance_set_surface_override_material(get_instance(), p_surface, surface_override_materials[p_surface]->get_rid());
		} else {
			RenderingServer::get_singleton()->instance_set_surface_override_material(get_instance(), p_surface, RID());
		}
	}
}

Ref<Material> SubdivMeshInstance3D::get_surface_override_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surface_override_materials.size(), Ref<Material>());

	return surface_override_materials[p_surface];
}

void SubdivMeshInstance3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &SubdivMeshInstance3D::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &SubdivMeshInstance3D::get_mesh);

	ClassDB::bind_method(D_METHOD("set_skin"), &SubdivMeshInstance3D::set_skin);
	ClassDB::bind_method(D_METHOD("get_skin"), &SubdivMeshInstance3D::get_skin);

	ClassDB::bind_method(D_METHOD("set_skeleton_path"), &SubdivMeshInstance3D::set_skeleton_path);
	ClassDB::bind_method(D_METHOD("get_skeleton_path"), &SubdivMeshInstance3D::get_skeleton_path);

	ClassDB::bind_method(D_METHOD("set_subdiv_level"), &SubdivMeshInstance3D::set_subdiv_level);
	ClassDB::bind_method(D_METHOD("get_subdiv_level"), &SubdivMeshInstance3D::get_subdiv_level);

	ClassDB::bind_method(D_METHOD("_update_skinning"), &SubdivMeshInstance3D::_update_skinning);
	ClassDB::bind_method(D_METHOD("_mesh_changed"), &SubdivMeshInstance3D::_mesh_changed);

	ClassDB::bind_method(D_METHOD("get_blend_shape_value", "blend_shape_idx"), &SubdivMeshInstance3D::get_blend_shape_value);
	ClassDB::bind_method(D_METHOD("set_blend_shape_value", "blend_shape_idx", "value"), &SubdivMeshInstance3D::set_blend_shape_value);

	ClassDB::bind_method(D_METHOD("get_surface_override_material_count"), &SubdivMeshInstance3D::get_surface_override_material_count);
	ClassDB::bind_method(D_METHOD("set_surface_override_material"), &SubdivMeshInstance3D::set_surface_override_material);
	ClassDB::bind_method(D_METHOD("get_surface_override_material"), &SubdivMeshInstance3D::get_surface_override_material);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "TopologyDataMesh"), "set_mesh", "get_mesh");
	ADD_GROUP("Skeleton", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skin", PROPERTY_HINT_RESOURCE_TYPE, "Skin"), "set_skin", "get_skin");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton_path", "get_skeleton_path");
	ADD_GROUP("Subdivision", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdiv_level", PROPERTY_HINT_RANGE, "0,6"), "set_subdiv_level", "get_subdiv_level");
	ADD_GROUP("", "");
}

SubdivMeshInstance3D::SubdivMeshInstance3D() {
	subdiv_mesh = NULL;
	subdiv_level = 0;
}

SubdivMeshInstance3D::~SubdivMeshInstance3D() {
	if (subdiv_mesh) {
		SubdivisionServer::get_singleton()->destroy_subdivision_mesh(subdiv_mesh);
	}
	subdiv_mesh = NULL;
}
