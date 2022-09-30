#ifndef BAKED_SUBDIV_MESH_INSTANCE_3D_H
#define BAKED_SUBDIV_MESH_INSTANCE_3D_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "resources/baked_subdiv_mesh.hpp"
using namespace godot;

class BakedSubdivMeshInstance3D : public MeshInstance3D {
	GDCLASS(BakedSubdivMeshInstance3D, MeshInstance3D)

protected:
	Ref<BakedSubdivMesh> get_mesh() {
		return MeshInstance3D::get_mesh();
	}
	void _set_correct_base();

	static void _bind_methods();
	void _notification(int p_what);

public:
	BakedSubdivMeshInstance3D();
	~BakedSubdivMeshInstance3D();
};
#endif