#include "subdiv_mesh_instance_3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"

#include "godot_cpp/classes/surface_tool.hpp"

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "subdivision_server.hpp"

void SubdivMeshInstance3D::set_mesh(const Ref<SubdivDataMesh> &p_mesh) {
	mesh = p_mesh;
	//_initialize_helper_mesh();
	if (is_inside_tree()) {
		_update_subdiv();
	}
}

Ref<SubdivDataMesh> SubdivMeshInstance3D::get_mesh() const {
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

void SubdivMeshInstance3D::_update_subdiv() {
	if (mesh.is_null()) {
		if (subdiv_mesh) {
			SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
			ERR_FAIL_COND(!subdivision_server);
			subdivision_server->destroy_subdivision_mesh(subdiv_mesh);
			subdiv_mesh = NULL;
		}
		return;
	}

	if (!subdiv_mesh) {
		SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
		ERR_FAIL_COND(!subdivision_server);
		subdiv_mesh = Object::cast_to<SubdivisionMesh>(subdivision_server->create_subdivision_mesh(mesh, subdiv_level));
	} else {
		if (mesh.is_null()) {
			if (subdiv_mesh) {
				SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
				ERR_FAIL_COND(!subdivision_server);
				subdivision_server->destroy_subdivision_mesh(subdiv_mesh);
				subdiv_mesh = NULL;
			}
			return;
		}

		if (!subdiv_mesh) {
			SubdivisionServer *subdivision_server = SubdivisionServer::get_singleton();
			ERR_FAIL_COND(!subdivision_server);
			subdiv_mesh = Object::cast_to<SubdivisionMesh>(subdivision_server->create_subdivision_mesh(mesh, subdiv_level));
		} else {
			subdiv_mesh->update_subdivision(mesh, subdiv_level);
		}
	}

	if (skin_ref.is_valid()) {
		// Intialize from current skeleton pose
		_update_skinning();
	}
	set_base(subdiv_mesh->get_rid());
	update_gizmos();
	notify_property_list_changed();
}

void SubdivMeshInstance3D::set_subdiv_level(int p_level) {
	ERR_FAIL_COND(p_level < 0);
	subdiv_level = p_level;
	_update_subdiv();
}
int32_t SubdivMeshInstance3D::get_subdiv_level() {
	return subdiv_level;
}

void SubdivMeshInstance3D::_notification(int p_what) {
	switch (p_what) {
		case 10: { //Node::NOTIFICATION_ENTER_TREE{ //FIXME: no access to enum
			_resolve_skeleton_path();
			_update_subdiv();
		} break;
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
	int surface_count = mesh->get_surface_count();

	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		Array mesh_arrays = mesh->surface_get_arrays(surface_index);
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
	ERR_FAIL_COND(vertex_array.size() != mesh->surface_get_length(p_surface));
	const PackedInt32Array &index_array = mesh->surface_get_arrays(p_surface)[SubdivDataMesh::ARRAY_INDEX];
	subdiv_mesh->update_subdivision_vertices(p_surface, vertex_array, index_array);
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

void SubdivMeshInstance3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &SubdivMeshInstance3D::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &SubdivMeshInstance3D::get_mesh);

	ClassDB::bind_method(D_METHOD("set_skin", "skin"), &SubdivMeshInstance3D::set_skin);
	ClassDB::bind_method(D_METHOD("get_skin"), &SubdivMeshInstance3D::get_skin);

	ClassDB::bind_method(D_METHOD("set_skeleton_path", "skeleton_path"), &SubdivMeshInstance3D::set_skeleton_path);
	ClassDB::bind_method(D_METHOD("get_skeleton_path"), &SubdivMeshInstance3D::get_skeleton_path);

	ClassDB::bind_method(D_METHOD("set_subdiv_level", "subdiv_level"), &SubdivMeshInstance3D::set_subdiv_level);
	ClassDB::bind_method(D_METHOD("get_subdiv_level"), &SubdivMeshInstance3D::get_subdiv_level);

	ClassDB::bind_method(D_METHOD("_update_skinning"), &SubdivMeshInstance3D::_update_skinning);
	ClassDB::bind_method(D_METHOD("_resolve_skeleton_path"), &SubdivMeshInstance3D::_resolve_skeleton_path);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "SubdivDataMesh"), "set_mesh", "get_mesh");
	ADD_GROUP("Skeleton", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skin", PROPERTY_HINT_RESOURCE_TYPE, "Skin"), "set_skin", "get_skin");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton_path", "get_skeleton_path");
	ADD_GROUP("Subdivision", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "subdiv_level", PROPERTY_HINT_RANGE, "0,6"), "set_subdiv_level", "get_subdiv_level");
	ADD_GROUP("", "");
}

SubdivMeshInstance3D::SubdivMeshInstance3D() {
	subdiv_level = 0;
	subdiv_mesh = NULL;
}

SubdivMeshInstance3D::~SubdivMeshInstance3D() {
	SubdivisionServer::get_singleton()->destroy_subdivision_mesh(subdiv_mesh);
	subdiv_mesh = NULL;
}
