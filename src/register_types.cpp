#include "register_types.h"

#include <godot/gdnative_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

#include "gltf_quad_importer.hpp"
#include "importer_quad_mesh.hpp"
#include "quad_mesh_instance_3d.hpp"
#include "subdivision_mesh.hpp"
#include "subdivision_server.hpp"

using namespace godot;

static SubdivisionServer *_subdivision_server;

void gdextension_initialize(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		ClassDB::register_class<SubdivisionServer>();
		ClassDB::register_class<ImporterQuadMesh>();
		ClassDB::register_class<QuadMeshInstance3D>();
		ClassDB::register_class<SubdivisionMesh>();
		ClassDB::register_class<GLTFQuadImporter>();
		_subdivision_server = memnew(SubdivisionServer);
		Engine::get_singleton()->register_singleton("SubdivisionServer", _subdivision_server);
	}
}

void gdextension_terminate(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		Engine::get_singleton()->unregister_singleton("SubdivisionServer");
		memdelete(_subdivision_server);
	}
}

extern "C"
{
	GDNativeBool GDN_EXPORT gdextension_init(const GDNativeInterface *p_interface, const GDNativeExtensionClassLibraryPtr p_library, GDNativeInitialization *r_initialization)
	{
		godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

		init_obj.register_initializer(gdextension_initialize);
		init_obj.register_terminator(gdextension_terminate);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
