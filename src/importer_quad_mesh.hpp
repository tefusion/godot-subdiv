#ifndef IMPORTER_QUAD_MESH_H
#define IMPORTER_QUAD_MESH_H

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/binder_common.hpp>

// //#include "godot_cpp/classes/ref.hpp"
// //#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/resource.hpp"
// #include "godot_cpp/core/math.hpp"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/templates/vector.hpp"
#include <godot_cpp/classes/mesh.hpp>

using namespace godot;

class ImporterQuadMesh : public Resource {
	GDCLASS(ImporterQuadMesh, Resource);

	struct Surface {
		Array arrays;
		struct BlendShape {
			Array arrays;
		};
		Vector<BlendShape> blend_shape_data;
		Ref<Material> material;
		String name;
		int32_t flags = 0;
		//int32_t format;

		void split_normals(const Vector<int> &p_indices, const Vector<Vector3> &p_normals);
		static void _split_normals(Array &r_arrays, const Vector<int> &p_indices, const Vector<Vector3> &p_normals);
	};
	Vector<Surface> surfaces;

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
			vertex_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_VERTEX];
			normal_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_NORMAL];
			index_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_INDEX];
			uv_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_TEX_UV];
			uv_index_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_UV_INDEX];
			if (p_mesh_arrays[ImporterQuadMesh::ARRAY_BONES])
				bones_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_BONES];

			if (p_mesh_arrays[ImporterQuadMesh::ARRAY_WEIGHTS])
				weights_array = p_mesh_arrays[ImporterQuadMesh::ARRAY_WEIGHTS];
		}
	};

	ImporterQuadMesh();
	~ImporterQuadMesh();
	void add_surface(const Array &p_arrays, const Array &p_blend_shapes,
			const Ref<Material> &p_material, const String &p_name);
	Array generate_trimesh_arrays(int surface_index);
	Array get_helper_mesh_arrays(int p_surface);
	Array surface_get_arrays(int p_surface) const;
	String surface_get_name(int p_surface) const;
	void surface_set_name(int p_surface, const String &p_name);
	Ref<Material> surface_get_material(int p_surface) const;
	void surface_set_material(int p_surface, const Ref<Material> &p_material);
	void surface_set_current_vertex_array(int p_surface, const PackedVector3Array &p_vertex_array);
	PackedVector3Array surface_get_current_vertex_array(int p_surface, const PackedVector3Array &p_vertex_array);
	int32_t get_surface_count();
	int surface_get_length(int p_surface);
	void clear();
};

#endif