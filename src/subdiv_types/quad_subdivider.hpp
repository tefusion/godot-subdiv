#ifndef QUAD_SUBDIVIDER_H
#define QUAD_SUBDIVIDER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "subdivider.hpp"

class QuadSubdivider : public Subdivider {
	GDCLASS(QuadSubdivider, Subdivider);

public:
	QuadSubdivider();
	~QuadSubdivider();

protected:
	static void _bind_methods();
	virtual OpenSubdiv::Sdc::SchemeType _get_refiner_type() const override;
	virtual void _create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
			const int32_t p_level, const bool face_varying_data) override;
	virtual PackedVector3Array _calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array) override;
	virtual Array _get_triangle_arrays() const override;
	virtual Vector<int> _get_face_vertex_count() const override;
	virtual Array _get_direct_triangle_arrays() const override;
};

#endif