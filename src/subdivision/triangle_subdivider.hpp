#ifndef TRIANGLE_SUBDIVIDER_H
#define TRIANGLE_SUBDIVIDER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "subdivider.hpp"

class TriangleSubdivider : public Subdivider {
	GDCLASS(TriangleSubdivider, Subdivider);

public:
	TriangleSubdivider();
	~TriangleSubdivider();

protected:
	static void _bind_methods();
	virtual OpenSubdiv::Sdc::SchemeType _get_refiner_type() const override;
	virtual Array _get_triangle_arrays() const override;
	virtual Vector<int> _get_face_vertex_count() const override;
	virtual int32_t _get_vertices_per_face_count() const override;
	virtual Array _get_direct_triangle_arrays() const override;
};

#endif