#ifndef SUBDIVISION_SERVER_H
#define SUBDIVISION_SERVER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

//#include <godot_cpp/classes/mesh.hpp>

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

class SubdivisionMesh;
class TopologyDataMesh;

using namespace godot;
class SubdivisionServer : public Object {
	GDCLASS(SubdivisionServer, Object);
	static SubdivisionServer *singleton;

protected:
	static void _bind_methods();

public:
	static SubdivisionServer *get_singleton();
	SubdivisionMesh *create_subdivision_mesh(const Ref<TopologyDataMesh> &p_mesh, int32_t p_level);
	SubdivisionMesh *create_subdivision_mesh_with_rid(const Ref<TopologyDataMesh> &p_mesh, int32_t p_level, RID p_rid);
	void destroy_subdivision_mesh(Object *p_mesh_subdivision);
	SubdivisionServer();
	~SubdivisionServer();
};

#endif