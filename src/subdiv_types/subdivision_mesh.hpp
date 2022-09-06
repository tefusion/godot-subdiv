#ifndef SUBDIVISON_MESH_H
#define SUBDIVISON_MESH_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

#include "far/topologyDescriptor.h"
#include "resources/subdiv_data_mesh.hpp"

using namespace godot;

//SubdivisionMesh is only for subdividing ImporterQuadMeshes
class SubdivisionMesh : public RefCounted {
	GDCLASS(SubdivisionMesh, RefCounted);
	RID source_mesh; //ImporterQuadMesh
	RID subdiv_mesh; //generated triangle mesh
	int subdiv_vertex_count = 0;
	int current_level = -1;

	// OpenSubdiv::Far::TopologyRefiner *refiner;
protected:
	static void _bind_methods();

private:
	struct SubdivData {
		PackedVector3Array vertex_array;
		PackedVector3Array normal_array;
		PackedVector2Array uv_array;
		PackedInt32Array uv_index_array;
		PackedInt32Array index_array;
		PackedFloat32Array bones_array;
		PackedFloat32Array weights_array;

		int32_t index_count = 0;
		int32_t face_count = 0;
		int32_t vertex_count = 0;
		int32_t uv_count = 0;
		int32_t bone_count = 0;
		int32_t weight_count = 0;
		SubdivData(const Array &p_mesh_arrays);
		SubdivData(const PackedVector3Array &p_vertex_array, const PackedInt32Array &p_index_array);
	};

	enum Channels {
		UV = 0
	};
	void _create_subdiv_data(const Array &quad_arrays, SubdivData *subdiv_data);
	OpenSubdiv::Far::TopologyDescriptor _create_topology_descriptor(
			const SubdivData &subdiv, const int num_channels);
	OpenSubdiv::Far::TopologyRefiner *_create_topology_refiner(
			SubdivData *subdiv, const int32_t p_level, const int num_channels);

	void _create_subdivision_vertices(
			SubdivData *subdiv, OpenSubdiv::Far::TopologyRefiner *refiner, const int p_level, const bool face_varying_data);
	void _create_subdivision_faces(const SubdivData &subdiv, OpenSubdiv::Far::TopologyRefiner *refiner,
			const int32_t p_level, const bool face_varying_data, Array &subdiv_quad_arrays);
	PackedVector3Array _calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array);

public:
	SubdivisionMesh();
	~SubdivisionMesh();

	RID get_rid() const { return subdiv_mesh; }

	void update_subdivision(Ref<SubdivDataMesh> p_mesh, int p_level);
	void update_subdivision_vertices(int p_surface, const PackedVector3Array &new_vertex_array, const PackedInt32Array &index_array);
};

#endif