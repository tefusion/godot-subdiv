#ifndef SUBDIVIDER_H
#define SUBDIVIDER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/templates/vector.hpp"

#include "far/primvarRefiner.h"
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

public:
	enum Channels {
		UV = 0
	};

protected:
	TopologyData topology_data; //used for both passing to opensubdiv and storing result topology again (easier to handle level 0 like that)
	void subdivide(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals); //sets topology_data
	OpenSubdiv::Far::TopologyDescriptor _create_topology_descriptor(Vector<int> &subdiv_face_vertex_count,
			OpenSubdiv::Far::TopologyDescriptor::FVarChannel *channels, const int32_t p_format);
	OpenSubdiv::Far::TopologyRefiner *_create_topology_refiner(const int32_t p_level, const int num_channels);
	void _create_subdivision_vertices(OpenSubdiv::Far::TopologyRefiner *refiner, const int p_level, const int32_t p_format);
	void _create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
			const int32_t p_level, const int32_t p_format);
	PackedVector3Array _calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array);

	virtual OpenSubdiv::Sdc::SchemeType _get_refiner_type() const;
	virtual Vector<int> _get_face_vertex_count() const;
	virtual int32_t _get_vertices_per_face_count() const;
	virtual Array _get_triangle_arrays() const;
	//might be needed if actual topology data with not just quads OR triangles is used, otherwise just calls _get_triangle_arrays
	virtual Array _get_direct_triangle_arrays() const;

public:
	Array get_subdivided_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals); //Returns triangle faces for rendering
	Array get_subdivided_topology_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals); //returns actual face data
	Subdivider();
	~Subdivider();
};

#endif