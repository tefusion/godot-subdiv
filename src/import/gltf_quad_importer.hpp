#ifndef GLTF_QUAD_IMPORTER_H
#define GLTF_QUAD_IMPORTER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

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

	struct SurfaceVertexArrays { //of imported triangle mesh
		PackedVector3Array vertex_array;
		PackedVector3Array normal_array;
		PackedInt32Array index_array;
		PackedVector2Array uv_array;
		PackedInt32Array bones_array; //could be float or int array after docs
		PackedFloat32Array weights_array;
		SurfaceVertexArrays(const Array &p_mesh_arrays);
		SurfaceVertexArrays(){};
	};

	GLTFQuadImporter::QuadSurfaceData _remove_duplicate_vertices(const SurfaceVertexArrays &surface);
	Array _generate_packed_blend_shapes(const Array &tri_blend_shapes, const PackedInt32Array &mesh_index_array, const PackedVector3Array &mesh_vertex_array);
	void _merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array);
	PackedInt32Array _generate_uv_index_array(PackedVector2Array &uv_array);
	Array generate_quad_mesh_arrays(const SurfaceVertexArrays &surface);

public:
	GLTFQuadImporter();
	~GLTFQuadImporter();
	void convert_meshinstance_to_quad(Object *p_meshinstance);
	void convert_importer_meshinstance_to_quad(Object *p_meshinstance);
};

#endif