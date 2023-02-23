#ifndef SUBDIVIDER_H
#define SUBDIVIDER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/templates/vector.hpp"

#include "far/primvarRefiner.h"
#include "far/stencilTable.h"
#include "far/stencilTableFactory.h"
#include "far/topologyDescriptor.h"

using namespace godot;

class Subdivider : public RefCounted {
	GDCLASS(Subdivider, RefCounted);

protected:
	static void _bind_methods();

protected:
	struct TopologyData {
		PackedVector3Array vertex_array;
		PackedVector3Array normal_array;
		PackedVector2Array uv_array;
		PackedInt32Array uv_index_array;
		PackedInt32Array index_array;
		PackedInt32Array bones_array;
		PackedFloat32Array weights_array;

		int32_t vertex_count_per_face = 0;
		int32_t index_count = 0;
		int32_t face_count = 0;
		int32_t vertex_count = 0;
		int32_t uv_count = 0;
		int32_t bone_count = 0;
		int32_t weight_count = 0;
		TopologyData(const Array &p_mesh_arrays, int32_t p_format, int32_t p_face_verts);
		TopologyData() {}
	};

	const OpenSubdiv::Far::StencilTable *vertex_table;
	int subdiv_level = 0;

public:
	enum Channels {
		UV = 0
	};

protected:
	TopologyData topology_data; //used for both passing to opensubdiv and storing result topology again (easier to handle level 0 like that)
	void subdivide(const PackedVector3Array &vertex_array, bool calculate_normals); //sets topology_data

	/**
	 * @brief 
	 * 
	 * @param subdiv_face_vertex_count 
	 * @param channels 
	 * @param p_format 
	 * @return OpenSubdiv::Far::TopologyDescriptor 
	 */
	OpenSubdiv::Far::TopologyDescriptor _create_topology_descriptor(Vector<int> &subdiv_face_vertex_count,
			OpenSubdiv::Far::TopologyDescriptor::FVarChannel *channels, const int32_t p_format);

	/**
	 * @brief Call at constructor, initializes needed tables/data for future subdivision
	 * 
	 * @param p_level 
	 * @param num_channels 
	 * @return OpenSubdiv::Far::TopologyRefiner* 
	 */
	OpenSubdiv::Far::TopologyRefiner *_create_topology_refiner(const int32_t p_level, const int num_channels);

	/**
	 * @brief Creates opensubdiv stencil table based on refiner
	 * 
	 * @param refiner 
	 * @param interpolation_mode 
	 * @return const OpenSubdiv::Far::StencilTable* 
	 */
	const OpenSubdiv::Far::StencilTable *_create_stenctil_table(OpenSubdiv::Far::TopologyRefiner *refiner, OpenSubdiv::Far::StencilTableFactory::Mode interpolation_mode);
	
	/**
	 * @brief Updates index array to subdivided form, call before first vertex subdivision
	 * 
	 * @param refiner 
	 * @param p_level subdivision level
	 * @param p_format Surface format (see ArrayMesh)
	 */
	void _set_index_arrays(OpenSubdiv::Far::TopologyRefiner *refiner,
			const int32_t p_level, int32_t p_format);
	
	/**
	 * @brief Called before first subdivision, sets Varying data array (UV1, Bone/weights currently)
	 * 
	 * @param refiner 
	 * @param p_format Surface format (see ArrayMesh)
	 * @param p_level subdivision level
	 */
	void _initialize_subdivided_mesh_array(OpenSubdiv::Far::TopologyRefiner *refiner, const int32_t p_format, int p_level); //handles UV's, bones, etc.

	/**
	 * @brief Initialize stensils for subdivision
	 * 
	 */
	void _create_subdivision_vertices();
	/**
	 * @brief Returns normal array, called before triangulating on subdivided mesh processed by opensubdiv
	 * 
	 * @param quad_vertex_array Vertex array after subdivision
	 * @param quad_index_array Index array after subdivision
	 * @return PackedVector3Array 
	 */
	PackedVector3Array _calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array);

	/**
	 * @brief Returns used subdivision scheme, default SCHEME_CATMARK
	 * 
	 * @return OpenSubdiv::Sdc::SchemeType 
	 */
	virtual OpenSubdiv::Sdc::SchemeType _get_refiner_type() const;

	/**
	 * @brief Returns the amount of vertices indexed to each face
	 * 
	 * @return Vector<int> 
	 */
	virtual Vector<int> _get_face_vertex_count() const;

	/**
	 * @brief Returns number of vertices each face has, e.g. for a quad mesh 4
	 * Only use with consistent topology mesh, otherwise overwrite _get_face_vertex_count
	 * 
	 * @return int32_t 
	 */
	virtual int32_t _get_vertices_per_face_count() const;
	/**
	 * @brief Returns arrays usable with ArrayMesh
	 * 
	 * @return Array 
	 */

	virtual Array _get_triangle_arrays() const;
	virtual Array _get_direct_triangle_arrays() const;

public:
	Array get_subdivided_arrays(const PackedVector3Array &vertex_array, bool calculate_normals); //Returns triangle faces for rendering
	Array get_subdivided_topology_arrays(const PackedVector3Array &vertex_array, bool calculate_normals); //returns actual face data
	void initialize(const Array &p_arrays, int p_level, int32_t p_format);
	Subdivider();
	Subdivider(const Array &p_arrays, int p_level, int32_t p_format);
	~Subdivider();
};

#endif