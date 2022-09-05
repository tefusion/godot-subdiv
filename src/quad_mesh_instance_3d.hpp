#ifndef QUAD_MESH_INSTANCE_3D_H
#define QUAD_MESH_INSTANCE_3D_H

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/binder_common.hpp>

#include "godot_cpp/classes/geometry_instance3d.hpp"
#include "godot_cpp/classes/immediate_mesh.hpp"
#include "godot_cpp/classes/skin.hpp"
#include "godot_cpp/classes/skin_reference.hpp"
#include "importer_quad_mesh.hpp"
#include "subdivision_mesh.hpp"

class QuadMeshInstance3D : public GeometryInstance3D {
	GDCLASS(QuadMeshInstance3D, GeometryInstance3D)

protected:
	Ref<ImporterQuadMesh> mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path = NodePath("..");

	Vector<Ref<Material>> surface_materials;
	int32_t subdiv_level;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_mesh(const Ref<ImporterQuadMesh> &p_mesh);
	Ref<ImporterQuadMesh> get_mesh() const;

	void set_skin(const Ref<Skin> &p_skin);
	Ref<Skin> get_skin() const;

	void set_surface_material(int p_idx, const Ref<Material> &p_material);
	Ref<Material> get_surface_material(int p_idx) const;

	void set_skeleton_path(const NodePath &p_path);
	NodePath get_skeleton_path() const;

	void set_subdiv_level(int p_level);
	int32_t get_subdiv_level();

	QuadMeshInstance3D();
	~QuadMeshInstance3D();

protected:
	SubdivisionMesh *subdiv_mesh;
	void _update_subdiv();
	void _update_skinning();
	void _resolve_skeleton_path();
	void _update_subdiv_mesh_vertices(int p_surface, const PackedVector3Array &vertex_array);
	//void _initialize_helper_mesh();
};
;

#endif