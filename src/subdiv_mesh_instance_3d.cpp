#include "subdiv_mesh_instance_3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"

#include "godot_cpp/classes/surface_tool.hpp"

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "subdivision_server.hpp"

//editor functions
void SubdivMeshInstance3D::_notification(int p_what) {
	switch (p_what) {
		case 13: { //NOTIFICATION_READY
			Callable callable = Callable(this, "_check_mesh_instance_changes");
			this->connect("property_list_changed", Callable(this, "_check_mesh_instance_changes"));
			break;
		}
		case 10: { //NOTIFICATION_ENTER_TREE
			_init_cached_data_array();
			_resolve_skeleton_path();

			break;
		}
		//FIXME: notification enter tree was too late for surface materials to create subdiv mesh, this solution isn't best cause parented!=in scene
		case 18: { //NOTIFICATION_PARENTED
			_update_subdiv();
			set_base(subdiv_mesh->get_rid());
			update_gizmos();

			break;
		}
		case 19: { //NOTIFIACTION_UNPARENTED
			if (subdiv_mesh) {
				SubdivisionServer::get_singleton()->destroy_subdivision_mesh(subdiv_mesh);
			}
			subdiv_mesh = NULL;
		}
	}
}

void SubdivMeshInstance3D::_get_property_list(List<PropertyInfo> *p_list) const {
	List<String> blend_shape_name_list;
	for (const KeyValue<StringName, int> &E : blend_shape_names) {
		blend_shape_name_list.push_back(String(E.key));
	}
	blend_shape_name_list.sort();
	for (int blend_shape_pos = 0; blend_shape_pos < blend_shape_name_list.size(); blend_shape_pos++) {
		char *blend_shape_name = new char[blend_shape_name_list[blend_shape_pos].length() + 1];
		memcpy(blend_shape_name, blend_shape_name_list[blend_shape_pos].utf8().get_data(), blend_shape_name_list[blend_shape_pos].length() + 1);
		p_list->push_back(PropertyInfo(Variant::FLOAT, blend_shape_name, PROPERTY_HINT_RANGE, "-1,1,0.00001"));
	}
}

//only for blendshapes setter
bool SubdivMeshInstance3D::_set(const StringName &p_name, const Variant &p_value) {
	if (!get_instance().is_valid()) {
		return false;
	}
	HashMap<StringName, int>::Iterator found = blend_shape_names.find(p_name);
	if (found) {
		set_blend_shape_value(found->value, p_value);
		return true;
	}
	return false;
}
//only for blendshapes getter
bool SubdivMeshInstance3D::_get(const StringName &p_name, Variant &return_value) const {
	if (!get_instance().is_valid()) {
		return false;
	}
	HashMap<StringName, int>::ConstIterator found = blend_shape_names.find(p_name);
	if (found) {
		return_value = get_blend_shape_value(found->value);
		return true;
	}
	return false;
}

//start of non editor stuff
void SubdivMeshInstance3D::set_subdiv_level(int p_level) {
	ERR_FAIL_COND(p_level < 0);
	subdiv_level = p_level;

	if (is_inside_tree()) {
		_update_subdiv();
		if (skin_ref.is_valid()) {
			_update_skinning();
		}
	}
}

int32_t SubdivMeshInstance3D::get_subdiv_level() {
	return subdiv_level;
}

float SubdivMeshInstance3D::get_blend_shape_value(int p_blend_shape) const {
	ERR_FAIL_COND_V(get_mesh().is_null(), 0.0);
	ERR_FAIL_INDEX_V(p_blend_shape, (int)blend_shape_values.size(), 0);
	return blend_shape_values[p_blend_shape];
}

void SubdivMeshInstance3D::set_blend_shape_value(int p_blend_shape, float p_value) {
	ERR_FAIL_COND(get_mesh().is_null());
	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_values.size());
	float last_value = blend_shape_values.get(p_blend_shape);
	blend_shape_values.set(p_blend_shape, p_value);

	//update cached array, currently only updates vertex values
	if (is_inside_tree()) {
		ERR_FAIL_COND(cached_data_array.is_empty());
		for (int surface_idx = 0; surface_idx < get_mesh()->get_surface_count(); surface_idx++) {
			const Array &blend_shape_offset = get_mesh()->surface_get_single_blend_shape_data_array(surface_idx, p_blend_shape);
			const Array &base_shape = get_mesh()->surface_get_data_arrays(surface_idx);
			const PackedVector3Array &blend_shape_vertex_offsets = blend_shape_offset[SubdivDataMesh::ARRAY_VERTEX];
			Array &target_surface = cached_data_array.write[surface_idx];
			PackedVector3Array surface_vertex_array = target_surface[SubdivDataMesh::ARRAY_VERTEX]; //maybe reference, array makes that hard
			ERR_FAIL_COND(surface_vertex_array.size() != blend_shape_vertex_offsets.size());
			float offset = p_value - last_value;
			for (int vertex_idx = 0; vertex_idx < surface_vertex_array.size(); vertex_idx++) {
				surface_vertex_array[vertex_idx] += offset * (blend_shape_vertex_offsets[vertex_idx]);
			}
			target_surface[SubdivDataMesh::ARRAY_VERTEX] = surface_vertex_array;
		}
		_update_subdiv();
		if (skin_ref.is_valid()) {
			_update_skinning();
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
		ERR_CONTINUE(!(get_mesh()->_surface_get_format(surface_index) & Mesh::ARRAY_FORMAT_BONES));
		Array mesh_arrays = _get_cached_data_array(surface_index);
		PackedVector3Array vertex_array = mesh_arrays[SubdivDataMesh::ARRAY_VERTEX];
		const PackedInt32Array &bones_array = mesh_arrays[SubdivDataMesh::ARRAY_BONES];
		const PackedFloat32Array &weights_array = mesh_arrays[SubdivDataMesh::ARRAY_WEIGHTS];

		ERR_FAIL_COND(bones_array.size() != weights_array.size() || bones_array.size() != vertex_array.size() * 4);

		//TODO: add format to quad mesh
		// ERR_CONTINUE(0 == (format_read & Mesh::ARRAY_FORMAT_BONES));
		// ERR_CONTINUE(0 == (format_read & Mesh::ARRAY_FORMAT_WEIGHTS));

		for (int vertex_index = 0; vertex_index < vertex_array.size(); ++vertex_index) {
			float bone_weights[4];
			int b[4];
			for (int bone_idx = 0; bone_idx < 4; bone_idx++) {
				bone_weights[bone_idx] = weights_array[vertex_index * 4 + bone_idx];
				b[bone_idx] = bones_array[vertex_index * 4 + bone_idx];
			}

			Transform3D transform;
			transform.origin =
					bone_weights[0] * bone_transforms[b[0]].origin +
					bone_weights[1] * bone_transforms[b[1]].origin +
					bone_weights[2] * bone_transforms[b[2]].origin +
					bone_weights[3] * bone_transforms[b[3]].origin;

			transform.basis =
					bone_weights[0] * bone_transforms[b[0]].basis +
					bone_weights[1] * bone_transforms[b[1]].basis +
					bone_weights[2] * bone_transforms[b[2]].basis +
					bone_weights[3] * bone_transforms[b[3]].basis;

			vertex_array[vertex_index] = transform.xform(vertex_array[vertex_index]);
		}
		_update_subdiv_mesh_vertices(surface_index, vertex_array);
	}
}

//calls subdiv mesh function to rerun subdivision with custom vertex array
void SubdivMeshInstance3D::_update_subdiv_mesh_vertices(int p_surface, const PackedVector3Array &vertex_array) {
	ERR_FAIL_COND(vertex_array.size() != get_mesh()->surface_get_length(p_surface));
	const PackedInt32Array &index_array = get_mesh()->surface_get_data_arrays(p_surface)[SubdivDataMesh::ARRAY_INDEX];
	subdiv_mesh->update_subdivision_vertices(p_surface, vertex_array, index_array);
}

void SubdivMeshInstance3D::_resolve_skeleton_path() {
	Ref<SkinReference> new_skin_reference;
	if (!get_skeleton_path().is_empty()) {
		Skeleton3D *skeleton = get_node<Skeleton3D>(get_skeleton_path());
		skin_skeleton_path = get_skeleton_path();
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
		Skeleton3D *skeleton = get_node<Skeleton3D>(get_skeleton_path());
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
	if (!get_mesh().is_valid()) {
		subdiv_mesh->clear();
		return;
	}
	if (!subdiv_mesh) {
		SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
		ERR_FAIL_COND(!subdivision_server);
		subdiv_mesh = Object::cast_to<SubdivisionMesh>(subdivision_server->create_subdivision_mesh(get_mesh(), subdiv_level));
	} else {
		subdiv_mesh->_update_subdivision(get_mesh(), subdiv_level, cached_data_array);
	}
	if (skin_ref.is_valid()) {
		_update_skinning();
	}
}

void SubdivMeshInstance3D::_check_mesh_instance_changes() {
	if (!get_mesh().is_valid() || get_mesh()->get_rid() != subdiv_mesh->source_mesh) {
		_init_cached_data_array();
		_update_subdiv();
	} else if (skin_skeleton_path != get_skeleton_path()) {
		_resolve_skeleton_path();
		_update_skinning();
	}
}

//returns mesh->surface_get_data_arrays if cached_data_array empty
Array SubdivMeshInstance3D::_get_cached_data_array(int p_surface) const {
	if (cached_data_array.size() == 0) {
		return get_mesh()->surface_get_data_arrays(p_surface);
	} else {
		ERR_FAIL_INDEX_V(p_surface, cached_data_array.size(), Array());
		return cached_data_array[p_surface];
	}
}

//also inits blend shape data values
void SubdivMeshInstance3D::_init_cached_data_array() {
	cached_data_array.clear();
	blend_shape_names.clear();
	blend_shape_values.clear();
	if (get_mesh().is_valid()) {
		for (int surface_idx = 0; surface_idx < get_mesh()->get_surface_count(); surface_idx++) {
			cached_data_array.push_back(get_mesh()->surface_get_data_arrays(surface_idx));
		}
		if (get_mesh()->get_blend_shape_data_count() != blend_shape_names.size()) {
			for (int blend_shape_idx = 0; blend_shape_idx < get_mesh()->get_blend_shape_data_count(); blend_shape_idx++) {
				blend_shape_names.insert(get_mesh()->_get_blend_shape_name(blend_shape_idx), blend_shape_idx);
				blend_shape_values.push_back(0);
			}
		}

		notify_property_list_changed();
	}
}

void SubdivMeshInstance3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &SubdivMeshInstance3D::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &SubdivMeshInstance3D::get_mesh);

	ClassDB::bind_method(D_METHOD("set_subdiv_level"), &SubdivMeshInstance3D::set_subdiv_level);
	ClassDB::bind_method(D_METHOD("get_subdiv_level"), &SubdivMeshInstance3D::get_subdiv_level);

	ClassDB::bind_method(D_METHOD("_update_skinning"), &SubdivMeshInstance3D::_update_skinning);
	ClassDB::bind_method(D_METHOD("_check_mesh_instance_changes"), &SubdivMeshInstance3D::_check_mesh_instance_changes);

	ClassDB::bind_method(D_METHOD("get_blend_shape_value", "blend_shape_idx"), &SubdivMeshInstance3D::get_blend_shape_value);
	ClassDB::bind_method(D_METHOD("set_blend_shape_value", "blend_shape_idx", "value"), &SubdivMeshInstance3D::set_blend_shape_value);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdiv_level", PROPERTY_HINT_RANGE, "0,6"), "set_subdiv_level", "get_subdiv_level");
	ADD_GROUP("Blend Shapes", "");
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
