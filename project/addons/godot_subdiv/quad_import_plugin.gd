# import_plugin.gd
@tool
extends EditorImportPlugin


func _get_importer_name():
	return "godot-subdiv.importer"

func _get_visible_name():
	return "Godot Subdiv Importer"

func _get_recognized_extensions():
	return ["glb", "gltf"]

func _get_save_extension():
	return "scn"

func _get_resource_type():
	return "PackedScene"

func _get_preset_count():
	return 1

func _get_preset_name(i):
	return "Default"
	
func _get_priority():
	return 2
	
func _get_import_order():
	return IMPORT_ORDER_SCENE
	
func _get_option_visibility(path, option_name, options):
	return true
	
func _get_import_options(path, preset_index):
	var options=[
		{
		"name": "import_as",
		"property_hint": PROPERTY_HINT_ENUM,
		"default_value": "SubdivMeshInstance3D",
		"hint_string": "SubdivMeshInstance3D,BakedSubdivMesh (subdivision in mesh),ArrayMesh (no subdivision)"
		},
		{
		"name": "sudbivision_level",
		"default_value": 0,
		"property_hint": PROPERTY_HINT_RANGE,
		"hint_string": "0,6"
		}
	]
	return options

func _convert_string_to_import_mode(enum_string: String) -> int:
	match enum_string:
		"SubdivMeshInstance3D":
			return GLTFQuadImporter.SUBDIV_MESHINSTANCE
		"BakedSubdivMesh (subdivision in mesh)":
			return GLTFQuadImporter.BAKED_SUBDIV_MESH
		"ArrayMesh (no subdivision)":
			return GLTFQuadImporter.ARRAY_MESH
		_:
			return -1
		
		

func _convert_mesh_instances_recursively(node: Node, importer: GLTFQuadImporter, import_mode: int, subdiv_level: int):
	for i in node.get_children():
		if i is ImporterMeshInstance3D:
			importer.convert_importer_meshinstance_to_quad(i, import_mode, subdiv_level)
		
		_convert_mesh_instances_recursively(i, importer, import_mode, subdiv_level)

func _import(source_file, save_path, options, platform_variants, gen_files):
	var gltf := GLTFDocument.new()
	var gltf_state := GLTFState.new()
	gltf.append_from_file(source_file, gltf_state, 0, 0)
	var node=gltf.generate_scene(gltf_state)
	#set root name, otherwise throws name is empty
	node.name=source_file.get_file().get_basename()
	
	var importer=GLTFQuadImporter.new()
	
	var import_mode=_convert_string_to_import_mode(options["import_as"])
	var subdiv_level=options["sudbivision_level"]

	if node!=null:
		_convert_mesh_instances_recursively(node, importer, import_mode, subdiv_level)
	else:
		print("GLTF importer failed, so could not run convert code")
	
	var packed=PackedScene.new()
	packed.pack(node)
	node.free()
	
	ResourceSaver.save(packed, save_path+".scn")
	



