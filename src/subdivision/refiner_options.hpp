#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "sdc/options.h"

using namespace godot;

typedef OpenSubdiv::Sdc::Options OpenSubdivSdcOptions;

/////////////////////////////////////////////////////
// Necessary reexports to be able to expose them nicely in Godot
class SubdivVtxBoundaryInterpolation : public RefCounted {
	GDCLASS(SubdivVtxBoundaryInterpolation, RefCounted)
public:
	enum VtxBoundaryInterpolation {
		VTX_BOUNDARY_NONE = OpenSubdivSdcOptions::VTX_BOUNDARY_NONE,
		VTX_BOUNDARY_EDGE_ONLY = OpenSubdivSdcOptions::VTX_BOUNDARY_EDGE_ONLY,
		VTX_BOUNDARY_EDGE_AND_CORNER = OpenSubdivSdcOptions::VTX_BOUNDARY_EDGE_AND_CORNER
	};

protected:
	static void _bind_methods();
};

class SubdivFVarLinearInterpolation : public RefCounted {
	GDCLASS(SubdivFVarLinearInterpolation, RefCounted)
public:
	enum FVarLinearInterpolation {
		FVAR_LINEAR_NONE = OpenSubdivSdcOptions::FVAR_LINEAR_NONE,
		FVAR_LINEAR_CORNERS_ONLY = OpenSubdivSdcOptions::FVAR_LINEAR_CORNERS_ONLY,
		FVAR_LINEAR_CORNERS_PLUS1 = OpenSubdivSdcOptions::FVAR_LINEAR_CORNERS_PLUS1,
		FVAR_LINEAR_CORNERS_PLUS2 = OpenSubdivSdcOptions::FVAR_LINEAR_CORNERS_PLUS2,
		FVAR_LINEAR_BOUNDARIES = OpenSubdivSdcOptions::FVAR_LINEAR_BOUNDARIES,
		FVAR_LINEAR_ALL = OpenSubdivSdcOptions::FVAR_LINEAR_ALL
	};

protected:
	static void _bind_methods();
};

class SubdivCreasingMethod : public RefCounted {
	GDCLASS(SubdivCreasingMethod, RefCounted)
public:
	enum CreasingMethod {
		CREASE_UNIFORM = OpenSubdivSdcOptions::CREASE_UNIFORM,
		CREASE_CHAIKIN = OpenSubdivSdcOptions::CREASE_CHAIKIN
	};

protected:
	static void _bind_methods();
};

class SubdivTriangleSubdivision : public RefCounted {
	GDCLASS(SubdivTriangleSubdivision, RefCounted)
public:
	enum TriangleSubdivision {
		TRI_SUB_CATMARK = OpenSubdivSdcOptions::TRI_SUB_CATMARK,
		TRI_SUB_SMOOTH = OpenSubdivSdcOptions::TRI_SUB_SMOOTH
	};

protected:
	static void _bind_methods();
};
/////////////////////////////////////////////////////

class SubdivRefinerOptions : public RefCounted {
	GDCLASS(SubdivRefinerOptions, RefCounted)
private:
	OpenSubdivSdcOptions options;

public:
	SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation get_vtx_boundary_interpolation() const;
	void set_vtx_boundary_interpolation(SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation vtx_boundary_interpolation);

	SubdivFVarLinearInterpolation::FVarLinearInterpolation get_fvar_linear_interpolation() const;
	void set_fvar_linear_interpolation(SubdivFVarLinearInterpolation::FVarLinearInterpolation fvar_linear_interpolation);

	SubdivCreasingMethod::CreasingMethod get_creasing_method() const;
	void set_creasing_method(SubdivCreasingMethod::CreasingMethod creasing_method);

	SubdivTriangleSubdivision::TriangleSubdivision get_triangle_subdivision() const;
	void set_triangle_subdivision(SubdivTriangleSubdivision::TriangleSubdivision triangle_subdivision);

	OpenSubdivSdcOptions get_options() const;

protected:
	static void _bind_methods();
};

VARIANT_ENUM_CAST(SubdivVtxBoundaryInterpolation::VtxBoundaryInterpolation);
VARIANT_ENUM_CAST(SubdivFVarLinearInterpolation::FVarLinearInterpolation);
VARIANT_ENUM_CAST(SubdivCreasingMethod::CreasingMethod);
VARIANT_ENUM_CAST(SubdivTriangleSubdivision::TriangleSubdivision);
