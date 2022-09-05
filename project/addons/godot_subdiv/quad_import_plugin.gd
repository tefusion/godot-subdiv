# import_plugin.gd
@tool
extends EditorImportPlugin


func _get_importer_name():
	return "quad.gltf_importer"

func _get_visible_name():
	return "Quad gltf importer"

func _get_recognized_extensions():
	return ["glb", "gltf", "glbo"]

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
	
func _get_import_options(path, preset_index):
	return []

func _convert_mesh_instances_recursively(node: Node, importer: GLTFQuadImporter):
	for i in node.get_children():
		if i is MeshInstance3D:
			importer.get_quad_mesh_from_array_mesh(i.mesh)
				
		elif i is ImporterMeshInstance3D:
			importer.convert_importer_meshinstance_to_quad(i)
		
		_convert_mesh_instances_recursively(i, importer)

func _import(source_file, save_path, options, platform_variants, gen_files):
	var gltf := GLTFDocument.new()
	var gltf_state := GLTFState.new()
	gltf.append_from_file(source_file, gltf_state, 0, 0)
	var node=gltf.generate_scene(gltf_state)
	var importer=GLTFQuadImporter.new()

	if node!=null:
		_convert_mesh_instances_recursively(node, importer)
	else:
		print("GLTF importer failed, so could not run convert code")
	
	var packed=PackedScene.new()
	packed.pack(node)
	node.free()
	
	ResourceSaver.save(packed, save_path+".scn")
	



