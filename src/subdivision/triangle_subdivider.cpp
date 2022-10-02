#include "triangle_subdivider.hpp"

#include "godot_cpp/classes/mesh.hpp"

using namespace OpenSubdiv;

OpenSubdiv::Sdc::SchemeType TriangleSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_LOOP;
}

Array TriangleSubdivider::_get_triangle_arrays() const {
	Array subdiv_triangle_arrays;
	subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);
	PackedVector2Array uv_array;
	PackedVector3Array vertex_array;
	PackedInt32Array index_array;
	PackedVector3Array normal_array;
	PackedInt32Array bones_array;
	PackedFloat32Array weights_array;

	bool use_uv = topology_data.uv_array.size();
	bool use_bones = topology_data.weights_array.size();
	bool has_normals = topology_data.normal_array.size();

	for (int index = 0; index < topology_data.index_array.size(); index++) {
		if (use_uv) {
			uv_array.append(topology_data.uv_array[topology_data.uv_index_array[index]]);
		}
		vertex_array.append(topology_data.vertex_array[topology_data.index_array[index]]);
		if (has_normals) {
			normal_array.append(topology_data.normal_array[topology_data.index_array[index]]);
		}
		if (use_bones) {
			for (int bone_index = 0; bone_index < 4; bone_index++) {
				bones_array.append(topology_data.bones_array[topology_data.index_array[index] * 4 + bone_index]);
				weights_array.append(topology_data.weights_array[topology_data.index_array[index] * 4 + bone_index]);
			}
		}
		index_array.append(index);
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