#pragma once

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/array.hpp"

using namespace godot;

class LocalMesh : public RefCounted {
	GDCLASS(LocalMesh, RefCounted);

	/**
	 * @brief RID of Mesh on Rendering Server
	 *
	 */
	RID local_mesh;
	/**
	 * @brief Seperate variable to locally have surface count,
	 * used to not have to ask RenderingServer for surface count
	 *
	 */
	int num_surfaces = 0;
	/**
	 * @brief Used to store offsets in vertex stride for each surface, inner vector has to be size Mesh::ARRAY_MAX
	 *
	 */
	Vector<Vector<uint32_t>> mesh_surface_offsets;
	Vector<uint32_t> vertex_strides;

protected:
	static void
	_bind_methods();

public:
	LocalMesh();
	~LocalMesh();

	RID get_rid() const;

	/**
	 * @brief Add surface to mesh on Rendering Server
	 *
	 * @param p_arrays
	 * @param p_lods
	 * @param p_material
	 * @param p_name
	 * @param p_format
	 */
	void add_surface(const Array &p_arrays, const Dictionary &p_lods, const Ref<Material> &p_material,
			const String &p_name, int32_t p_format);

	/**
	 * @brief Update vertex, tangent and normal of a surface
	 *
	 * @param surface_idx
	 * @param p_arrays vertex, normal, tangent
	 */
	void update_surface_vertices(int surface_idx, const Array &p_arrays);

	/**
	 * @brief Clears mesh:
	 *
	 * Needs to be called if surface removed, can not remove single surfaces
	 */
	void clear_surfaces();
};