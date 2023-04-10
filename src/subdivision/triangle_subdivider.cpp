#include "triangle_subdivider.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/surface_tool.hpp"

using namespace OpenSubdiv;

OpenSubdiv::Sdc::SchemeType TriangleSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_LOOP;
}

Array TriangleSubdivider::_get_triangle_arrays() const {
	Ref<SurfaceTool> st;
	st.instantiate();

	bool use_uv = topology_data.uv_array.size();
	bool use_bones = topology_data.weights_array.size();
	bool has_normals = topology_data.normal_array.size();

	st->begin(Mesh::PRIMITIVE_TRIANGLES);
	for (int index = 0; index < topology_data.index_array.size(); index++) {
		if (use_uv) {
			st->set_uv(topology_data.uv_array[topology_data.uv_index_array[index]]);
		}
		if (has_normals) {
			st->set_normal(topology_data.normal_array[topology_data.index_array[index]]);
		}
		if (use_bones) {
			PackedInt32Array bones_array;
			PackedFloat32Array weights_array;
			for (int bone_index = 0; bone_index < 4; bone_index++) {
				bones_array.append(topology_data.bones_array[topology_data.index_array[index] * 4 + bone_index]);
				weights_array.append(topology_data.weights_array[topology_data.index_array[index] * 4 + bone_index]);
			}
			st->set_bones(bones_array);
			st->set_weights(weights_array);
		}
		st->add_vertex(topology_data.vertex_array[topology_data.index_array[index]]);
		st->add_index(index);
	}

	if (has_normals && use_uv) {
		st->generate_tangents();
	}

	return st->commit_to_arrays();
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