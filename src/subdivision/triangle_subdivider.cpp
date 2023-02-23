#include "triangle_subdivider.hpp"
#include "godot_cpp/classes/mesh.hpp"

using namespace OpenSubdiv;

OpenSubdiv::Sdc::SchemeType TriangleSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_LOOP;
}

Array TriangleSubdivider::_get_triangle_arrays() const {
	const int RESULT_VERTEX_COUNT = topology_data.index_array.size();
	Array subdiv_triangle_arrays;
	subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);

	PackedVector3Array vertex_array;
	vertex_array.resize(RESULT_VERTEX_COUNT);
	PackedInt32Array index_array;
	index_array.resize(RESULT_VERTEX_COUNT);

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

	for (int index = 0; index < topology_data.index_array.size(); index++) {
		if (use_uv) {
			uv_array[index] = topology_data.uv_array[topology_data.uv_index_array[index]];
		}
		if (has_normals) {
			normal_array[index] = topology_data.normal_array[topology_data.index_array[index]];
		}
		if (use_bones) {
			for (int bone_index = 0; bone_index < 4; bone_index++) {
				const int bone_weight_index = index * 4 + bone_index;
				bones_array[bone_weight_index] = topology_data.bones_array[topology_data.index_array[index] * 4 + bone_index];
				weights_array[bone_weight_index] = topology_data.weights_array[topology_data.index_array[index] * 4 + bone_index];
			}
		}
		vertex_array[index] = topology_data.vertex_array[topology_data.index_array[index]];
		index_array[index] = index;
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

Vector<int> TriangleSubdivider::_get_face_vertex_count() const {
	Vector<int> face_vertex_count;
	face_vertex_count.resize(topology_data.face_count);
	face_vertex_count.fill(3);
	return face_vertex_count;
};

int32_t TriangleSubdivider::_get_vertices_per_face_count() const {
	return 3;
}
Array TriangleSubdivider::_get_direct_triangle_arrays() const {
	return _get_triangle_arrays();
};

void TriangleSubdivider::_bind_methods() {
}

TriangleSubdivider::TriangleSubdivider() {
}

TriangleSubdivider::~TriangleSubdivider() {
}