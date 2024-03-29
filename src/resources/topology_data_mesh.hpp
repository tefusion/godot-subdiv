#pragma once

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/templates/vector.hpp"

using namespace godot;

class TopologyDataMesh : public Resource {
	GDCLASS(TopologyDataMesh, Resource);

public:
	/**
	 * @brief Contains custom arraymesh indices to contain uv index array
	 *
	 */
	enum ArrayType {
		ARRAY_VERTEX = Mesh::ARRAY_VERTEX,
		ARRAY_NORMAL = Mesh::ARRAY_NORMAL,
		ARRAY_TANGENT = Mesh::ARRAY_TANGENT,
		ARRAY_COLOR = Mesh::ARRAY_COLOR,
		ARRAY_TEX_UV = Mesh::ARRAY_TEX_UV,
		ARRAY_TEX_UV2 = Mesh::ARRAY_TEX_UV2,
		ARRAY_CUSTOM0 = Mesh::ARRAY_CUSTOM0,
		ARRAY_CUSTOM1 = Mesh::ARRAY_CUSTOM1,
		ARRAY_CUSTOM2 = Mesh::ARRAY_CUSTOM2,
		ARRAY_CUSTOM3 = Mesh::ARRAY_CUSTOM3,
		ARRAY_BONES = Mesh::ARRAY_BONES,
		ARRAY_WEIGHTS = Mesh::ARRAY_WEIGHTS,
		ARRAY_INDEX = Mesh::ARRAY_INDEX,
		ARRAY_UV_INDEX = Mesh::ARRAY_MAX, //just an index array for uv's (ARRAY_INDEX does not work with uv's anymore in favour of face connection)
		ARRAY_MAX = Mesh::ARRAY_MAX + 1
	};

	//TODO: Maybe make seperate class for storage:
	//need to make TRIANGLE_FLAT, QUAD_FLAT, TRIANGLE_SMOOTH, QUAD_SMOOTH and FULL_TOPOLOGY
	//since normals/tangents are stored differently based on shading,
	//full topology would have vertex array of actual topology vertices, but normals for each individual face vertex
	//the index array wouldn't be used and instead face_index_array=[[0,1,2,3], [2,5,6,9,8], ...], need to read on how to triangulate that
	enum TopologyType {
		TRIANGLE = 0,
		QUAD = 1
	};

protected:
	struct Surface {
		Array arrays;
		Array blend_shape_data; //Array[Array]
		Ref<Material> material;
		String name;
		int32_t flags = 0;
		AABB aabb;
		int32_t format;
		TopologyType topology_type;
		Dictionary lods;
	};
	Vector<Surface> surfaces;
	Array blend_shapes; //is Vector<StringName>, but that caused casting issues

	void _set_data(const Dictionary &p_data);
	Dictionary _get_data() const;
	static void _bind_methods();

public:
	struct TopologySurfaceData {
		godot::PackedVector3Array vertex_array;
		godot::PackedVector3Array normal_array;
		godot::PackedVector2Array uv_array;
		godot::PackedInt32Array uv_index_array;
		godot::PackedInt32Array index_array;
		godot::PackedFloat32Array bones_array;
		godot::PackedFloat32Array weights_array;
		TopologySurfaceData(Array p_mesh_arrays) {
			vertex_array = p_mesh_arrays[TopologyDataMesh::ARRAY_VERTEX];
			normal_array = p_mesh_arrays[TopologyDataMesh::ARRAY_NORMAL];
			index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_INDEX];
			uv_array = p_mesh_arrays[TopologyDataMesh::ARRAY_TEX_UV];
			uv_index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_UV_INDEX];
			if (p_mesh_arrays[TopologyDataMesh::ARRAY_BONES])
				bones_array = p_mesh_arrays[TopologyDataMesh::ARRAY_BONES];

			if (p_mesh_arrays[TopologyDataMesh::ARRAY_WEIGHTS])
				weights_array = p_mesh_arrays[TopologyDataMesh::ARRAY_WEIGHTS];
		}
	};

	/**
	 * @brief Return topology surface arrays
	 *
	 * @param p_surface
	 * @return Array
	 */
	Array surface_get_arrays(int p_surface) const;

	/**
	 * @brief add surface, Array needs to be ARRAY_MAX long and contain topology data
	 *
	 * @param p_arrays
	 * @param p_lods normal lods
	 * @param p_blend_shapes relative blendshapes
	 * @param p_material material
	 * @param p_name surface name
	 * @param p_format arrayformat from mesh
	 * @param p_topology_type triangle or quad
	 */
	void add_surface(const Array &p_arrays, const Dictionary &p_lods, const Array &p_blend_shapes,
			const Ref<Material> &p_material, const String &p_name, BitField<Mesh::ArrayFormat> p_format, TopologyType p_topology_type);
	/**
	 * @brief Just calls add_surface, more similar to ArrayMesh add_surface_from_arrays method and has default args
	 *
	 * @param p_topology_type
	 * @param p_arrays
	 * @param p_blend_shapes
	 * @param p_lods
	 * @param p_format
	 */
	void add_surface_from_arrays(TopologyType p_topology_type, const Array &p_arrays, const Array &p_blend_shapes = Array(), const Dictionary &p_lods = Dictionary(), BitField<Mesh::ArrayFormat> p_format = 1);

	/**
	 * @brief Getter for surface name
	 *
	 * @param p_surface Surface index
	 * @return String surface name
	 */
	String surface_get_name(int p_surface) const;
	/**
	 * @brief Setter for surface name
	 *
	 * @param p_surface surface index
	 * @param p_name new name
	 */
	void surface_set_name(int p_surface, const String &p_name);

	/**
	 * @brief Return vertex array length of surface
	 *
	 * @param p_surface surface index
	 * @return int vertex array length
	 */
	int surface_get_length(int p_surface);

	/**
	 * @brief Get the number of surfaces mesh has
	 *
	 * @return int64_t number of surfaces
	 */
	int64_t get_surface_count() const;

	/**
	 * @brief Returns surface format, same as ArrayMesh
	 *
	 * @param surface_index
	 * @return BitField<Mesh::ArrayFormat>
	 */
	BitField<Mesh::ArrayFormat> surface_get_format(int64_t surface_index) const;

	/**
	 * @brief Change material of surface
	 *
	 * @param surface_index
	 * @param material
	 */
	void surface_set_material(int64_t surface_index, const Ref<Material> &material);

	/**
	 * @brief Getter for material
	 *
	 * @param surface_index
	 * @return Ref<Material>
	 */
	Ref<Material> surface_get_material(int64_t surface_index) const;

	/**
	 * @brief Set surface topology type (triangle/quad)
	 *
	 * @param surface_index
	 * @param p_topology_type triangle or quad
	 */
	void surface_set_topology_type(int64_t surface_index, TopologyType p_topology_type);

	/**
	 * @brief Getter for topology type of surface
	 *
	 * @param surface_index
	 * @return TopologyType
	 */
	TopologyType surface_get_topology_type(int64_t surface_index) const;

	/**
	 * @brief Get amount of blendshapes
	 *
	 * @return int64_t
	 */

	Dictionary surface_get_lods(int64_t surface_index) const;

	int64_t get_blend_shape_count() const;

	/**
	 * @brief Get all blendshape arrays of surface
	 *
	 * @param surface_index
	 * @return Array
	 */
	Array surface_get_blend_shape_arrays(int64_t surface_index) const;

	/**
	 * @brief Only get single blendshape array of a surface
	 *
	 * @param surface_index
	 * @param blend_shape_idx
	 * @return Array
	 */
	Array surface_get_single_blend_shape_array(int64_t surface_index, int64_t blend_shape_idx) const;

	/**
	 * @brief Get the blend shape name at index
	 *
	 * @param index
	 * @return StringName
	 */

	StringName get_blend_shape_name(int64_t index) const;

	/**
	 * @brief Set the blend shape name at index
	 *
	 * @param index
	 * @param p_name
	 */
	void set_blend_shape_name(int64_t index, const StringName &p_name);
	/**
	 * @brief Add new blenshapes to mesh
	 *
	 * Only use after adding blend shapes to each surface
	 *
	 * @param p_name
	 */
	void add_blend_shape_name(const StringName &p_name);

	/**
	 * @brief Clears surfaces and blendshapes
	 *
	 */
	void clear();

	TopologyDataMesh();
	~TopologyDataMesh();
};

VARIANT_ENUM_CAST(TopologyDataMesh::TopologyType);
VARIANT_ENUM_CAST(TopologyDataMesh::ArrayType);