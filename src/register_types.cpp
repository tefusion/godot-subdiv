#include "register_types.h"

#include "godot/gdnative_interface.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"

#include "import/gltf_quad_importer.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdiv_mesh_instance_3d.hpp"
#include "subdiv_types/subdivision_mesh.hpp"
#include "subdivision_server.hpp"

#include "subdiv_types/quad_subdivider.hpp"
#include "subdiv_types/subdivider.hpp"
#ifdef TESTS_ENABLED
#include "subdiv_test.hpp"
#endif

using namespace godot;

static SubdivisionServer *_subdivision_server;

void gdextension_initialize(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<Subdivider>();
		ClassDB::register_class<QuadSubdivider>();
		ClassDB::register_class<SubdivisionServer>();
		ClassDB::register_class<SubdivMeshInstance3D>();
		ClassDB::register_class<SubdivisionMesh>();
		ClassDB::register_class<GLTFQuadImporter>();
		ClassDB::register_class<TopologyDataMesh>();

		_subdivision_server = memnew(SubdivisionServer);
		Engine::get_singleton()->register_singleton("SubdivisionServer", _subdivision_server);

#ifdef TESTS_ENABLED
		ClassDB::register_class<SubdivTest>();
		SubdivTest subdiv_test;
		subdiv_test.run_tests();
#endif
	}
}

void gdextension_terminate(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		Engine::get_singleton()->unregister_singleton("SubdivisionServer");
		memdelete(_subdivision_server);
	}
}

extern "C" {
GDNativeBool GDN_EXPORT gdextension_init(const GDNativeInterface *p_interface, const GDNativeExtensionClassLibraryPtr p_library, GDNativeInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	init_obj.register_initializer(gdextension_initialize);
	init_obj.register_terminator(gdextension_terminate);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
