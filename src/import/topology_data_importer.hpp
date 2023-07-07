#pragma once

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

#include "resources/topology_data_mesh.hpp"

using namespace godot;

class TopologyDataImporter : public Object {
	GDCLASS(TopologyDataImporter, Object);

private:
	/**
	 * @brief Struct used to simplfiy working with arrays of TopologyDataMesh
	 *
	 */
	struct TopologySurfaceData {
		godot::PackedVector3Array vertex_array;
		godot::PackedVector3Array normal_array;
		godot::PackedVector2Array uv_array;
		godot::PackedInt32Array index_array;
		godot::PackedInt32Array bones_array;
		godot::PackedFloat32Array weights_array;
	};

	/**
	 * @brief  Struct for easier editing of arrays from ArrayMesh
	 *
	 */
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

	/**
	 * @brief Find and remove vertices in vertex_array at same position. This edits the other arrays accordingly and then
	 * returns the TopologyDataArrays (which means faces are connected and not just floating triangles)
	 *
	 * @details This can lead to data loss / wrong results if two verticse that weren't connected before triangulating
	 * are at the same position.
	 *
	 *
	 * @param surface
	 * @param format
	 * @return TopologyDataImporter::TopologySurfaceData
	 */
	TopologyDataImporter::TopologySurfaceData _remove_duplicate_vertices(const SurfaceVertexArrays &surface, int32_t format);
	/**
	 * @brief Uses the mesh_vertex_array to merge blendshape arrays the exact same way
	 *
	 * @param tri_blend_shapes
	 * @param mesh_index_array
	 * @param mesh_vertex_array
	 * @return Array
	 */
	Array _generate_packed_blend_shapes(const Array &tri_blend_shapes,
			const PackedInt32Array &mesh_index_array, const PackedVector3Array &mesh_vertex_array);

	/**
	 * @brief
	 *
	 * @param index_array Topology Index Array after removing duplicate vertices
	 * @param uv_array Array of original UV's. UV's are in most cases different and part of the reason why
	 * the Triangles even got split up at export even when they are at the same position
	 * @param format
	 * @return true The mesh is a QuadMesh
	 * @return false The mesh is not a QuadMesh and didn't merge faces -> fallback to TriangleMesh
	 */
	bool _merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array, int32_t format);
	/**
	 * @brief Generates minimal needed UV index array (as vertex index array would cause data to be lost)
	 *
	 * @param uv_array
	 * @return PackedInt32Array
	 */
	PackedInt32Array _generate_uv_index_array(PackedVector2Array &uv_array);
	/**
	 * @brief Goes through all of the above methods (remove_duplicate, merge_to_quads) and saves the result in surface_arrays
	 *
	 * @param surface Surfaces of triangulated Mesh
	 * @param format
	 * @param surface_arrays Free Array where the result can be stored
	 * @return TopologyDataMesh::TopologyType
	 */
	TopologyDataMesh::TopologyType _generate_topology_surface_arrays(const SurfaceVertexArrays &surface, int32_t format, Array &surface_arrays);

	/**
	 * @brief Checks what arrays are not null and generates a format based on that
	 *
	 * @param arrays
	 * @return int32_t
	 */
	int32_t generate_fake_format(const Array &arrays) const;

	/**
	 * @brief Replaces the ImporterMeshInstance with a MeshInstance (used for ArrayMesh import)
	 *
	 * @param importer_mesh_instance_object
	 * @return Object*
	 */
	Object *_replace_importer_mesh_instance_with_mesh_instance(Object *importer_mesh_instance_object);

protected:
	static void _bind_methods();

public:
	enum ImportMode {
		SUBDIV_MESHINSTANCE = 0,
		BAKED_SUBDIV_MESH = 1,
		ARRAY_MESH = 2,
		IMPORTER_MESH = 3
	};

	/**
	 * @brief Replaces the ImporterMeshInstance3D with a SubdivMeshInstance3D
	 *
	 * @param p_meshinstance
	 * @param import_mode
	 * @param subdiv_level
	 */
	void convert_importer_meshinstance_to_subdiv(Object *p_meshinstance, ImportMode import_mode, int32_t subdiv_level);
};

VARIANT_ENUM_CAST(TopologyDataImporter::ImportMode);