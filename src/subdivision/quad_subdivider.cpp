#include "quad_subdivider.hpp"

#include "godot_cpp/classes/mesh.hpp"

using namespace OpenSubdiv;

OpenSubdiv::Sdc::SchemeType QuadSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_CATMARK;
}

Array QuadSubdivider::_get_triangle_arrays() const {
	Array subdiv_triangle_arrays;
	subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);
	PackedVector2Array uv_array;
	PackedVector3Array vertex_array;
	PackedInt32Array index_array;
	PackedVector3Array normal_array;
	PackedInt32Array bones_array;
	PackedFloat32Array weights_array;

	bool use_uv = topology_data.uv_array.size();
	bool use_bones = topology_data.bones_array.size();
	bool has_normals = topology_data.normal_array.size();

	for (int quad_index = 0; quad_index < topology_data.index_array.size(); quad_index += 4) {
		//add vertices part of quad

		//after for loop unshared0 vertex will be at the positon quad_index in the new vertex_array in the SurfaceTool
		for (int single_quad_index = quad_index; single_quad_index < quad_index + 4; single_quad_index++) {
			if (use_uv) {
				uv_array.append(topology_data.uv_array[topology_data.uv_index_array[single_quad_index]]);
			}
			vertex_array.append(topology_data.vertex_array[topology_data.index_array[single_quad_index]]);
			if (has_normals) {
				normal_array.append(topology_data.normal_array[topology_data.index_array[single_quad_index]]);
			}
			if (use_bones) {
				for (int bone_index = 0; bone_index < 4; bone_index++) {
					bones_array.append(topology_data.bones_array[topology_data.index_array[single_quad_index] * 4 + bone_index]);
					weights_array.append(topology_data.weights_array[topology_data.index_array[single_quad_index] * 4 + bone_index]);
				}
			}

		} //unshared0, shared0, unshared1, shared1

		//add triangle 1 with unshared0
		index_array.append(quad_index);
		index_array.append(quad_index + 1);
		index_array.append(quad_index + 3);

		//add triangle 2 with unshared1
		index_array.append(quad_index + 1);
		index_array.append(quad_index + 2);
		index_array.append(quad_index + 3);
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