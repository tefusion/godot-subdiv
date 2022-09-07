#ifndef SUBDIV_MESH_INSTANCE_3D_H
#define QUAD_MESH_INSTANCE_3D_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/immediate_mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/skin.hpp"
#include "godot_cpp/classes/skin_reference.hpp"
#include "resources/subdiv_data_mesh.hpp"
#include "subdiv_types/subdivision_mesh.hpp"

class SubdivMeshInstance3D : public MeshInstance3D {
	GDCLASS(SubdivMeshInstance3D, MeshInstance3D)

	void set_mesh(const Ref<Mesh> &p_mesh) {
		ERR_FAIL_COND_MSG(!p_mesh->is_class("SubdivDataMesh"), "This class only supports meshes of the type SubdivDataMesh.");
		MeshInstance3D::set_mesh(p_mesh);
	}

	Ref<SubdivDataMesh> get_mesh() const {
		return MeshInstance3D::get_mesh();
	}

protected:
	SubdivisionMesh *subdiv_mesh;
	int32_t subdiv_level;
	Ref<SkinReference> skin_ref; //godot cpp currently does not give access to protected skin ref variable
	Ref<Skin> skin_internal;
	NodePath skin_skeleton_path; //needed for checking changes

	Vector<Array> cached_data_array; //array of surfaces after blend shapes are applied, if empty (if no blendshapes) getter will return normal data array
	HashMap<StringName, int> blend_shape_names;
	Vector<float> blend_shape_values;

	void _resolve_skeleton_path();
	void _update_subdiv();
	void _check_mesh_instance_changes(); //function that gets called when property list changes
	Array _get_cached_data_array(int p_surface) const;
	void _init_cached_data_array();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &return_value) const;

public:
	int32_t get_subdiv_level();
	void set_subdiv_level(int p_level);

	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	//TODO: remove if MeshInstance gives access to skin ref
	NodePath get_skeleton_path() {
		return MeshInstance3D::get_skeleton_path();
	};
	Ref<Skin> get_skin() const {
		return MeshInstance3D::get_skin();
	};

	SubdivMeshInstance3D();
	~SubdivMeshInstance3D();

protected:
	void _update_skinning();
	void _update_subdiv_mesh_vertices(int p_surface, const PackedVector3Array &vertex_array);
	//void _initialize_helper_mesh();
};
;

#endif