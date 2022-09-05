#include <subdivision_server.hpp>

#include "subdivision_mesh.hpp"
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "importer_quad_mesh.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

SubdivisionServer *SubdivisionServer::singleton = nullptr;

SubdivisionServer *SubdivisionServer::get_singleton() {
	return singleton;
}

SubdivisionServer::SubdivisionServer() {
	singleton = this;
}

SubdivisionServer::~SubdivisionServer() {
	singleton = nullptr;
}

void SubdivisionServer::_bind_methods() {
	ClassDB::bind_static_method("SubdivisionServer", D_METHOD("get_singleton"), &SubdivisionServer::get_singleton);
	ClassDB::bind_method(D_METHOD("create_subdivision_mesh"), &SubdivisionServer::create_subdivision_mesh);
	ClassDB::bind_method(D_METHOD("destroy_subdivision_mesh"), &SubdivisionServer::destroy_subdivision_mesh);
}

SubdivisionMesh *SubdivisionServer::create_subdivision_mesh(const Ref<ImporterQuadMesh> &p_mesh, int32_t p_level) {
	SubdivisionMesh *subdiv_mesh = memnew(SubdivisionMesh);
	subdiv_mesh->update_subdivision(p_mesh, p_level);

	return subdiv_mesh;
}

// afaik you can't pass the actual Object class like SubdivisionMesh here, since this is only for freeing no casting needed
void SubdivisionServer::destroy_subdivision_mesh(Object *subdiv_mesh) {
	if (subdiv_mesh != nullptr) {
		memdelete(subdiv_mesh);
	}
}
