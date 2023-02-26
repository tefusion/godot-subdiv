@tool
extends EditorPlugin

var resource_import_plugin
var editor_scene_import_modifier

func _enter_tree():
	#This adds custom options to scene importer, remove if you don't want subdivision options for every mesh
	editor_scene_import_modifier=preload("res://addons/godot_subdiv/scene_import_modifier_plugin.gd").new()
	add_scene_post_import_plugin(editor_scene_import_modifier, true)

func _exit_tree():
	remove_scene_post_import_plugin(editor_scene_import_modifier)
	editor_scene_import_modifier=null
