#include "quad_subdivider.hpp"
#include "godot_cpp/classes/mesh.hpp"

using namespace OpenSubdiv;

OpenSubdiv::Sdc::SchemeType QuadSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_CATMARK;
}

Array QuadSubdivider::_get_triangle_arrays() const {
	const int RESULT_VERTEX_COUNT = topology_data.index_array.size();
	Array subdiv_triangle_arrays;
	subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertex_array;
	vertex_array.resize(RESULT_VERTEX_COUNT);
	PackedInt32Array index_array;
	index_array.resize(RESULT_VERTEX_COUNT * 6 / 4);

	PackedVector2Array uv_array;
	PackedVector3Array normal_array;
	PackedFloat32Array tangent_array;

	PackedInt32Array bones_array;
	PackedFloat32Array weights_array;

	const bool use_uv = topology_data.uv_array.size();
	const bool use_bones = topology_data.bones_array.size();
	const bool has_normals = topology_data.normal_array.size();

	if (use_uv) {
		uv_array.resize(RESULT_VERTEX_COUNT);
	}
	if (has_normals) {
		normal_array.resize(RESULT_VERTEX_COUNT);
		if (use_uv) {
			tangent_array.resize(RESULT_VERTEX_COUNT * 4);
		}
	}
	if (use_bones) {
		bones_array.resize(RESULT_VERTEX_COUNT * 4);
		weights_array.resize(RESULT_VERTEX_COUNT * 4);
	}

	for (int quad_index = 0; quad_index < RESULT_VERTEX_COUNT; quad_index += 4) {
		//after for loop unshared0 vertex will be at the positon quad_index in the new vertex_array in the SurfaceTool
		for (int single_quad_index = quad_index; single_quad_index < quad_index + 4; single_quad_index++) {
			if (use_uv) {
				uv_array[single_quad_index] = topology_data.uv_array[topology_data.uv_index_array[single_quad_index]];
			}
			if (has_normals) {
				normal_array[single_quad_index] = topology_data.normal_array[topology_data.index_array[single_quad_index]];
			}
			if (use_bones) {
				for (int bone_index = 0; bone_index < 4; bone_index++) {
					const int bone_weight_index = single_quad_index * 4 + bone_index;
					bones_array[bone_weight_index] = topology_data.bones_array[topology_data.index_array[single_quad_index] * 4 + bone_index];
					weights_array[bone_weight_index] = topology_data.weights_array[topology_data.index_array[single_quad_index] * 4 + bone_index];
				}
			}
			vertex_array[single_quad_index] = topology_data.vertex_array[topology_data.index_array[single_quad_index]];
		} //unshared0, shared0, unshared1, shared1

		const int triangle_index = quad_index * 6 / 4;
		//add triangle 1 with unshared0
		index_array[triangle_index] = quad_index;
		index_array[triangle_index + 1] = quad_index + 1;
		index_array[triangle_index + 2] = quad_index + 3;

		//add triangle 2 with unshared1
		index_array[triangle_index + 3] = quad_index + 1;
		index_array[triangle_index + 4] = quad_index + 2;
		index_array[triangle_index + 5] = quad_index + 3;
	}

	subdiv_triangle_arrays[Mesh::ARRAY_VERTEX] = vertex_array;
	subdiv_triangle_arrays[Mesh::ARRAY_INDEX] = index_array;
	if (use_uv) {
		subdiv_triangle_arrays[Mesh::ARRAY_TEX_UV] = uv_array;
	}
	if (has_normals) {
		subdiv_triangle_arrays[Mesh::ARRAY_NORMAL] = normal_array;
	}
	if (use_bones) {
		subdiv_triangle_arrays[Mesh::ARRAY_BONES] = bones_array;
		subdiv_triangle_arrays[Mesh::ARRAY_WEIGHTS] = weights_array;
	}

	if (has_normals && use_uv) {
		//TODO: generate tangents
		subdiv_triangle_arrays[Mesh::ARRAY_TANGENT] = tangent_array;
	}
	return subdiv_triangle_arrays;
}

Vector<int> QuadSubdivider::_get_face_vertex_count() const {
	Vector<int> face_vertex_count;
	face_vertex_count.resize(topology_data.face_count);
	face_vertex_count.fill(4);
	return face_vertex_count;
};

int32_t QuadSubdivider::_get_vertices_per_face_count() const {
	return 4;
}
Array QuadSubdivider::_get_direct_triangle_arrays() const {
	return _get_triangle_arrays();
};

void QuadSubdivider::_bind_methods() {
}

QuadSubdivider::QuadSubdivider() {
}

QuadSubdivider::~QuadSubdivider() {
}