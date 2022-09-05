#ifndef GLTF_QUAD_IMPORTER_H
#define GLTF_QUAD_IMPORTER_H

#include "godot_cpp/classes/importer_mesh_instance3d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "subdiv_structs.hpp"
#include <opensubdiv/far/topologyDescriptor.h>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
using namespace godot;

class GLTFQuadImporter : public Object {
	GDCLASS(GLTFQuadImporter, Object);

protected:
	static void
	_bind_methods();

private:
	struct QuadSurfaceData {
		godot::PackedVector3Array vertex_array;
		godot::PackedVector3Array normal_array;
		godot::PackedVector2Array uv_array;
		godot::PackedInt32Array index_array;
		godot::PackedInt32Array bones_array;
		godot::PackedFloat32Array weights_array;
	};
	GLTFQuadImporter::QuadSurfaceData _remove_duplicate_vertices(const SurfaceVertexArrays &surface);
	void _merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array);
	PackedInt32Array _generate_uv_index_array(PackedVector2Array &uv_array);

public:
	GLTFQuadImporter();
	~GLTFQuadImporter();
	void convert_meshinstance_to_quad(MeshInstance3D *p_meshinstance);
	void convert_importer_meshinstance_to_quad(Object *p_meshinstance);

private:
	void print_index_array(const SurfaceVertexArrays &surface);
};

#endif