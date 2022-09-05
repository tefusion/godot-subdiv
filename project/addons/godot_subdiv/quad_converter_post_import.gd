@tool
extends EditorScenePostImport


func _post_import(scene):
	var importer=GLTFQuadImporter.new()
	_convert_mesh_instances_recursively(scene, importer)
	return scene


func _convert_mesh_instances_recursively(node: Node, importer: GLTFQuadImporter):
	for i in node.get_children():
		if i is MeshInstance3D:
			importer.convert_meshinstance_to_quad(i)
		
		_convert_mesh_instances_recursively(i, importer)
