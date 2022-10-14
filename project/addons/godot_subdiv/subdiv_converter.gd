
var importer: TopologyDataImporter
var import_mode: int
var subdiv_level: int

func _init(import_mode_string: String, p_subdiv_level: int):
	self.importer=TopologyDataImporter.new()
	self.import_mode=_convert_string_to_import_mode(import_mode_string)
	self.subdiv_level=p_subdiv_level
	
func _convert_string_to_import_mode(enum_string: String) -> int:
	match enum_string:
		"SubdivMeshInstance3D":
			return TopologyDataImporter.SUBDIV_MESHINSTANCE
		"BakedSubdivMesh (bake at runtime)":
			return TopologyDataImporter.BAKED_SUBDIV_MESH
		"ArrayMesh (bake at import)":
			return TopologyDataImporter.ARRAY_MESH
		"ImporterMesh (bake at import)":
			return TopologyDataImporter.IMPORTER_MESH
		_:
			return -1

func convert_importer_mesh_instances_recursively(node: Node):
	for i in node.get_children():
		convert_importer_mesh_instances_recursively(i)
		if i is ImporterMeshInstance3D:
			importer.convert_importer_meshinstance_to_subdiv(i, import_mode, subdiv_level)
		elif i is AnimationPlayer:
			if import_mode==TopologyDataImporter.SUBDIV_MESHINSTANCE:
				convert_animation_blend_shape_tracks(i)
				
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
