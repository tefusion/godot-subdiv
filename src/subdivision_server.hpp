#ifndef SUBDIVISION_SERVER_H
#define SUBDIVISION_SERVER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

//#include <godot_cpp/classes/mesh.hpp>

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

class SubdivisionMesh;
class ImporterQuadMesh;

using namespace godot;
class SubdivisionServer : public Object {
	GDCLASS(SubdivisionServer, Object);
	static SubdivisionServer *singleton;

protected:
	static void _bind_methods();

public:
	static SubdivisionServer *get_singleton();
	SubdivisionMesh *create_subdivision_mesh(const Ref<ImporterQuadMesh> &p_mesh, int32_t p_level);
	void destroy_subdivision_mesh(Object *p_mesh_subdivision);
	SubdivisionServer();
	~SubdivisionServer();
};

#endif