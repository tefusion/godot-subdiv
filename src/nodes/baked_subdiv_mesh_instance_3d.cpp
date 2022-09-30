#include "baked_subdiv_mesh_instance_3d.hpp"

void BakedSubdivMeshInstance3D::_set_correct_base() {
	WARN_PRINT_ONCE("BakedSubdivMeshInstance3D is just a temporary solution. When get_rid becomes virtual access in GDExtensions it will be removed and will work together with MeshInstance3D.");
	set_base(get_mesh()->get_rid());
}

void BakedSubdivMeshInstance3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			this->connect("property_list_changed", Callable(this, "_set_correct_base"));
			break;
		}
		case NOTIFICATION_ENTER_TREE: {
			_set_correct_base();
			break;
		}
	}
}

BakedSubdivMeshInstance3D::BakedSubdivMeshInstance3D() {}
BakedSubdivMeshInstance3D::~BakedSubdivMeshInstance3D() {}
void BakedSubdivMeshInstance3D::_bind_methods() {
	ClassDB::bind_method("_set_correct_base", &BakedSubdivMeshInstance3D::_set_correct_base);
}