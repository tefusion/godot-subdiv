#include "register_types.h"

#include "gdextension_interface.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"

#include "import/topology_data_importer.hpp"
#include "nodes/subdiv_mesh_instance_3d.hpp"
#include "resources/baked_subdiv_mesh.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/subdivision_baker.hpp"
#include "subdivision/subdivision_mesh.hpp"
#include "subdivision/subdivision_server.hpp"

#include "subdivision/quad_subdivider.hpp"
#include "subdivision/subdivider.hpp"
#include "subdivision/triangle_subdivider.hpp"
#ifdef TESTS_ENABLED
#include "subdiv_test.hpp"
#endif

using namespace godot;

static SubdivisionServer *_subdivision_server;

void gdextension_initialize(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<Subdivider>();
		ClassDB::register_class<QuadSubdivider>();
		ClassDB::register_class<TriangleSubdivider>();

		ClassDB::register_class<SubdivisionServer>();
		ClassDB::register_class<SubdivisionMesh>();
		ClassDB::register_class<SubdivisionBaker>();

		ClassDB::register_class<SubdivMeshInstance3D>();

		ClassDB::register_class<TopologyDataImporter>();

		ClassDB::register_class<TopologyDataMesh>();
		ClassDB::register_class<BakedSubdivMesh>();

		_subdivision_server = memnew(SubdivisionServer);
		Engine::get_singleton()->register_singleton("SubdivisionServer", _subdivision_server);

#ifdef TESTS_ENABLED
		ClassDB::register_class<SubdivTest>();
		Ref<SubdivTest> subdiv_test = memnew(SubdivTest);
		subdiv_test->run_tests();
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
// Initialization.
GDExtensionBool GDE_EXPORT gdextension_init(const GDExtensionInterface *p_interface, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	init_obj.register_initializer(gdextension_initialize);
	init_obj.register_terminator(gdextension_terminate);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
