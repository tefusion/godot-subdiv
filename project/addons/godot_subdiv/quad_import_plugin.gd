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
		"hint_string": "SubdivMeshInstance3D,BakedSubdivMesh (bake at runtime),ArrayMesh (bake at import)"
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
		"BakedSubdivMesh (bake at runtime)":
			return GLTFQuadImporter.BAKED_SUBDIV_MESH
		"ArrayMesh (bake at import)":
			return GLTFQuadImporter.ARRAY_MESH
		_:
			return -1
		
		

func _convert_mesh_instances_recursively(node: Node, importer: GLTFQuadImporter, import_mode: int, subdiv_level: int):
	for i in node.get_children():
		if i is ImporterMeshInstance3D:
			importer.convert_importer_meshinstance_to_quad(i, import_mode, subdiv_level)
		elif i is AnimationPlayer:
			if import_mode==GLTFQuadImporter.SUBDIV_MESHINSTANCE:
				convert_animation_blend_shape_tracks(i)
				
		
		_convert_mesh_instances_recursively(i, importer, import_mode, subdiv_level)
		

#blend shape tracks don't work with SubdivMeshInstance3D
func convert_animation_blend_shape_tracks(anim_player: AnimationPlayer):
	for animation_library_name in anim_player.get_animation_library_list():
		var anim_lib:AnimationLibrary=anim_player.get_animation_library(animation_library_name)
		for animation_name in anim_lib.get_animation_list():
			var animation: Animation=anim_lib.get_animation(animation_name)
			for track_idx in range(0, animation.get_track_count()):
				if animation.track_get_type(track_idx)==Animation.TYPE_BLEND_SHAPE:
					var fake_anim=animation.add_track(Animation.TYPE_VALUE)
					var old_track_path=animation.track_get_path(track_idx)
					var node_name=String(old_track_path.get_name(0))
					var blend_shape_name=String(old_track_path.get_subname(0))
					var new_track_path=node_name+":"+"blend_shapes/"+blend_shape_name
					animation.track_set_path(fake_anim, NodePath(new_track_path))
					
					for key_idx in range(0, animation.track_get_key_count(track_idx)):
						var val=animation.track_get_key_value(track_idx, key_idx)
						var time=animation.track_get_key_time(track_idx, key_idx)
						var transition=animation.track_get_key_transition(track_idx, key_idx)
						animation.track_insert_key(fake_anim, time, val, transition)
					animation.track_swap(track_idx, fake_anim)
					animation.remove_track(fake_anim)

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
	



