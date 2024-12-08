#include "refiner_options.hpp"

void SubdivVtxBoundaryInterpolation::_bind_methods() {
	BIND_ENUM_CONSTANT(VTX_BOUNDARY_NONE);
	BIND_ENUM_CONSTANT(VTX_BOUNDARY_EDGE_ONLY);
	BIND_ENUM_CONSTANT(VTX_BOUNDARY_EDGE_AND_CORNER);
}

void SubdivFVarLinearInterpolation::_bind_methods() {
	BIND_ENUM_CONSTANT(FVAR_LINEAR_NONE);
	BIND_ENUM_CONSTANT(FVAR_LINEAR_CORNERS_ONLY);
	BIND_ENUM_CONSTANT(FVAR_LINEAR_CORNERS_PLUS1);
	BIND_ENUM_CONSTANT(FVAR_LINEAR_CORNERS_PLUS2);
	BIND_ENUM_CONSTANT(FVAR_LINEAR_BOUNDARIES);
	BIND_ENUM_CONSTANT(FVAR_LINEAR_ALL);
}

void SubdivCreasingMethod::_bind_methods() {
	BIND_ENUM_CONSTANT(CREASE_UNIFORM);
	BIND_ENUM_CONSTANT(CREASE_CHAIKIN);
}

void SubdivTriangleSubdivision::_bind_methods() {
	BIND_ENUM_CONSTANT(TRI_SUB_CATMARK);
	BIND_ENUM_CONSTANT(TRI_SUB_SMOOTH);
}

SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation SubdivRefinerOptions::get_vtx_boundary_interpolation() const {
	return static_cast<SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation>(options.GetVtxBoundaryInterpolation());
}

void SubdivRefinerOptions::set_vtx_boundary_interpolation(SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation vtx_boundary_interpolation) {
	options.SetVtxBoundaryInterpolation(static_cast<OpenSubdiv::Sdc::Options::VtxBoundaryInterpolation>(vtx_boundary_interpolation));
}

SubdivFVarLinearInterpolation::FVarLinearInterpolation SubdivRefinerOptions::get_fvar_linear_interpolation() const {
	return static_cast<SubdivFVarLinearInterpolation::FVarLinearInterpolation>(options.GetFVarLinearInterpolation());
}

void SubdivRefinerOptions::set_fvar_linear_interpolation(SubdivFVarLinearInterpolation::FVarLinearInterpolation fvar_linear_interpolation) {
	options.SetFVarLinearInterpolation(static_cast<OpenSubdiv::Sdc::Options::FVarLinearInterpolation>(fvar_linear_interpolation));
}

SubdivCreasingMethod::CreasingMethod SubdivRefinerOptions::get_creasing_method() const {
	return static_cast<SubdivCreasingMethod::CreasingMethod>(options.GetCreasingMethod());
}

void SubdivRefinerOptions::set_creasing_method(SubdivCreasingMethod::CreasingMethod creasing_method) {
	options.SetCreasingMethod(static_cast<OpenSubdiv::Sdc::Options::CreasingMethod>(creasing_method));
}

SubdivTriangleSubdivision::TriangleSubdivision SubdivRefinerOptions::get_triangle_subdivision() const {
	return static_cast<SubdivTriangleSubdivision::TriangleSubdivision>(options.GetTriangleSubdivision());
}

void SubdivRefinerOptions::set_triangle_subdivision(SubdivTriangleSubdivision::TriangleSubdivision triangle_subdivision) {
	options.SetTriangleSubdivision(static_cast<OpenSubdiv::Sdc::Options::TriangleSubdivision>(triangle_subdivision));
}

OpenSubdivSdcOptions SubdivRefinerOptions::to_sdc() const {
	return options;
}

Ref<SubdivRefinerOptions> SubdivRefinerOptions::default_options() {
	Ref<SubdivRefinerOptions> options;
	options.instantiate();
	options->set_vtx_boundary_interpolation(SubdivVtxBoundaryInterpolation::VTX_BOUNDARY_EDGE_ONLY);
	return options;
}

void SubdivRefinerOptions::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_vtx_boundary_interpolation"), &SubdivRefinerOptions::get_vtx_boundary_interpolation);
	ClassDB::bind_method(D_METHOD("set_vtx_boundary_interpolation", "vtx_boundary_interpolation"), &SubdivRefinerOptions::set_vtx_boundary_interpolation);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vtx_boundary_interpolation"), "set_vtx_boundary_interpolation", "get_vtx_boundary_interpolation");

	ClassDB::bind_method(D_METHOD("get_fvar_linear_interpolation"), &SubdivRefinerOptions::get_fvar_linear_interpolation);
	ClassDB::bind_method(D_METHOD("set_fvar_linear_interpolation", "fvar_linear_interpolation"), &SubdivRefinerOptions::set_fvar_linear_interpolation);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fvar_linear_interpolation"), "set_fvar_linear_interpolation", "get_fvar_linear_interpolation");

	ClassDB::bind_method(D_METHOD("get_creasing_method"), &SubdivRefinerOptions::get_creasing_method);
	ClassDB::bind_method(D_METHOD("set_creasing_method", "creasing_method"), &SubdivRefinerOptions::set_creasing_method);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "creasing_method"), "set_creasing_method", "get_creasing_method");

	ClassDB::bind_method(D_METHOD("get_triangle_subdivision"), &SubdivRefinerOptions::get_triangle_subdivision);
	ClassDB::bind_method(D_METHOD("set_triangle_subdivision", "triangle_subdivision"), &SubdivRefinerOptions::set_triangle_subdivision);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "triangle_subdivision"), "set_triangle_subdivision", "get_triangle_subdivision");
}
