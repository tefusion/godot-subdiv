#ifndef SUBDIV_DATA_MESH_H
#define SUBDIV_DATA_MESH_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/templates/vector.hpp"

using namespace godot;

class SubdivisionMesh;
class SubdivDataMesh : public Mesh {
	GDCLASS(SubdivDataMesh, Mesh);

	virtual int64_t _get_surface_count() const override;
	virtual int64_t _surface_get_array_len(int64_t index) const override;
	virtual int64_t _surface_get_array_index_len(int64_t index) const override;
	virtual Array _surface_get_arrays(int64_t index) const;
	virtual TypedArray<Array> _surface_get_blend_shape_arrays(int64_t index) const override;
	virtual Dictionary _surface_get_lods(int64_t index) const override;
	virtual int64_t _surface_get_format(int64_t index) const override;
	virtual int64_t _surface_get_primitive_type(int64_t index) const override;
	virtual void _surface_set_material(int64_t index, const Ref<Material> &material) override;
	virtual Ref<Material> _surface_get_material(int64_t index) const override;
	virtual int64_t _get_blend_shape_count() const override;
	virtual StringName _get_blend_shape_name(int64_t index) const override;
	virtual void _set_blend_shape_name(int64_t index, const StringName &name) override;
	virtual void _add_blend_shape_name(const StringName &p_name);
	virtual AABB _get_aabb() const override;
	struct Surface {
		Array arrays;
		Array blend_shape_data; //Array[Array][SubdivDataMesh::ARRAY_MAX]
		Ref<Material> material;
		String name;
		int32_t flags = 0;
		AABB aabb;
		int32_t format;
	};
	Vector<Surface> surfaces;
	Array blend_shapes; //is Vector<StringName>, but that caused casting issues

protected:
	void _set_data(const Dictionary &p_data);
	Dictionary _get_data() const;
	static void _bind_methods();

public:
	//extra space for uv index array
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

	struct QuadSurfaceData {
		godot::PackedVector3Array vertex_array;
		godot::PackedVector3Array normal_array;
		godot::PackedVector2Array uv_array;
		godot::PackedInt32Array uv_index_array;
		godot::PackedInt32Array index_array;
		godot::PackedFloat32Array bones_array;
		godot::PackedFloat32Array weights_array;
		QuadSurfaceData(Array p_mesh_arrays) {
			vertex_array = p_mesh_arrays[SubdivDataMesh::ARRAY_VERTEX];
			normal_array = p_mesh_arrays[SubdivDataMesh::ARRAY_NORMAL];
			index_array = p_mesh_arrays[SubdivDataMesh::ARRAY_INDEX];
			uv_array = p_mesh_arrays[SubdivDataMesh::ARRAY_TEX_UV];
			uv_index_array = p_mesh_arrays[SubdivDataMesh::ARRAY_UV_INDEX];
			if (p_mesh_arrays[SubdivDataMesh::ARRAY_BONES])
				bones_array = p_mesh_arrays[SubdivDataMesh::ARRAY_BONES];

			if (p_mesh_arrays[SubdivDataMesh::ARRAY_WEIGHTS])
				weights_array = p_mesh_arrays[SubdivDataMesh::ARRAY_WEIGHTS];
		}
	};

	Array generate_trimesh_arrays(int surface_index) const;
	Array generate_trimesh_arrays_from_quad_arrays(const Array &quad_arrays, int32_t p_format) const;
	Array get_helper_mesh_arrays(int p_surface);
	Array surface_get_data_arrays(int p_surface) const;
	Array surface_get_blend_shape_data_arrays(int64_t surface_index) const;
	Array surface_get_single_blend_shape_data_array(int64_t surface_index, int64_t blend_shape_idx) const;
	int64_t get_blend_shape_data_count() const;

	void add_surface(const Array &p_arrays, const Array &p_blend_shapes,
			const Ref<Material> &p_material, const String &p_name, int32_t p_format);
	String surface_get_name(int p_surface) const;
	void surface_set_name(int p_surface, const String &p_name);
	void surface_set_current_vertex_array(int p_surface, const PackedVector3Array &p_vertex_array);
	PackedVector3Array surface_get_current_vertex_array(int p_surface, const PackedVector3Array &p_vertex_array);
	int surface_get_length(int p_surface);
	void clear();

	SubdivDataMesh();
	~SubdivDataMesh();
};

#endif