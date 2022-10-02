#ifndef GLTF_QUAD_IMPORTER_H
#define GLTF_QUAD_IMPORTER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

#include "resources/topology_data_mesh.hpp"

using namespace godot;

class GLTFQuadImporter : public Object {
	GDCLASS(GLTFQuadImporter, Object);

protected:
	static void
	_bind_methods();

public:
	enum ImportMode {
		SUBDIV_MESHINSTANCE = 0,
		BAKED_SUBDIV_MESH = 1,
		ARRAY_MESH = 2
	};

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

	GLTFQuadImporter::QuadSurfaceData _remove_duplicate_vertices(const SurfaceVertexArrays &surface, int32_t format);
	Array _generate_packed_blend_shapes(const Array &tri_blend_shapes,
			const PackedInt32Array &mesh_index_array, const PackedVector3Array &mesh_vertex_array);
	bool _merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array, int32_t format);
	PackedInt32Array _generate_uv_index_array(PackedVector2Array &uv_array);
	TopologyDataMesh::TopologyType _generate_topology_surface_arrays(const SurfaceVertexArrays &surface, int32_t format, Array &surface_arrays);

	int32_t generate_fake_format(const Array &arrays) const;
	Object *_replace_importer_mesh_instance_with_mesh_instance(Object *importer_mesh_instance_object);

public:
	GLTFQuadImporter();
	~GLTFQuadImporter();
	void convert_meshinstance_to_quad(Object *p_meshinstance);
	void convert_importer_meshinstance_to_quad(Object *p_meshinstance, ImportMode import_mode, int32_t subdiv_level);
};

VARIANT_ENUM_CAST(GLTFQuadImporter, ImportMode);

#endif