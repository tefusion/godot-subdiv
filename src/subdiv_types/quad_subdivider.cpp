#include "quad_subdivider.hpp"

#include "godot_cpp/classes/mesh.hpp"

using namespace OpenSubdiv;

PackedVector3Array QuadSubdivider::_calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array) {
	PackedVector3Array normals;
	normals.resize(quad_vertex_array.size());
	for (int f = 0; f < quad_index_array.size(); f += 4) {
		// // We will use the first three verts to calculate a normal
		const Vector3 &p_point1 = quad_vertex_array[quad_index_array[f]];
		const Vector3 &p_point2 = quad_vertex_array[quad_index_array[f + 1]];
		const Vector3 &p_point3 = quad_vertex_array[quad_index_array[f + 2]];
		Vector3 normal_calculated = (p_point1 - p_point3).cross(p_point1 - p_point2);
		normal_calculated.normalize();
		for (int n_pos = f; n_pos < f + 4; n_pos++) {
			int vertexIndex = quad_index_array[n_pos];
			normals[vertexIndex] += normal_calculated;
		}
	}
	//normalized accumulated normals
	for (int vertex_index = 0; vertex_index < normals.size(); ++vertex_index) {
		normals[vertex_index].normalize();
	}
	return normals;
}

OpenSubdiv::Sdc::SchemeType QuadSubdivider::_get_refiner_type() const {
	return OpenSubdiv::Sdc::SchemeType::SCHEME_CATMARK;
}
void QuadSubdivider::_create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
		const int32_t p_level, const bool face_varying_data) {
	PackedInt32Array index_array;
	PackedInt32Array uv_index_array;

	Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
	int face_count_out = last_level.GetNumFaces();
	int uv_index_offset = face_varying_data ? topology_data.uv_count - last_level.GetNumFVarValues(Channels::UV) : -1;
	int vertex_index_offset = topology_data.vertex_count - last_level.GetNumVertices();
	for (int face_index = 0; face_index < face_count_out; ++face_index) {
		int parent_face_index = last_level.GetFaceParentFace(face_index);
		for (int level_index = p_level - 1; level_index > 0; --level_index) {
			Far::TopologyLevel const &prev_level = refiner->GetLevel(level_index);
			parent_face_index = prev_level.GetFaceParentFace(parent_face_index);
		}

		Far::ConstIndexArray face_vertices = last_level.GetFaceVertices(face_index);

		ERR_FAIL_COND_MSG(face_vertices.size() != 4, "Faces need to be quads.");
		for (int face_vert_index = 0; face_vert_index < 4; face_vert_index++) {
			index_array.push_back(vertex_index_offset + face_vertices[face_vert_index]);
		}

		if (face_varying_data) {
			Far::ConstIndexArray face_uvs = last_level.GetFaceFVarValues(face_index, Channels::UV);
			for (int face_vert_index = 0; face_vert_index < 4; face_vert_index++) {
				uv_index_array.push_back(uv_index_offset + face_uvs[face_vert_index]);
			}
		}
	}
	topology_data.index_array = index_array;
	if (face_varying_data) {
		topology_data.uv_index_array = uv_index_array;
	}
}
Array QuadSubdivider::_get_triangle_arrays() const {
	Array subdiv_triangle_arrays;
	subdiv_triangle_arrays.resize(Mesh::ARRAY_MAX);
	PackedVector2Array uv_array;
	PackedVector3Array vertex_array;
	PackedInt32Array index_array;
	PackedVector3Array normal_array;

	bool use_uv = topology_data.uv_array.size();
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

	return subdiv_triangle_arrays;
}

void QuadSubdivider::_bind_methods() {
}

QuadSubdivider::QuadSubdivider() {
}

QuadSubdivider::~QuadSubdivider() {
}