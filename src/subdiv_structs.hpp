#ifndef SUBDIV_STRUCTS
#define SUBDIV_STRUCTS

#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"

using namespace godot;
struct Vertex {
	void Clear() { x = y = z = 0; }
	void AddWithWeight(Vertex const &src, float weight) {
		x += weight * src.x;
		y += weight * src.y;
		z += weight * src.z;
	}
	float x, y, z;
};

struct VertexUV {
	void Clear() {
		u = v = 0.0f;
	}

	void AddWithWeight(VertexUV const &src, float weight) {
		u += weight * src.u;
		v += weight * src.v;
	}

	// Basic 'uv' layout channel
	float u, v;
};

struct VertexBones {
	void Clear() {
		b0 = b1 = b2 = b3 = 0;
	}

	void AddWithWeight(VertexBones const &src, float weight) {
		b0 = src.b0;
		b1 = src.b1;
		b2 = src.b2;
		b3 = src.b3;
	}

	int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
};
struct VertexWeights {
	void Clear() {
		w0 = w1 = w2 = w3 = 0.0f;
	}

	void AddWithWeight(VertexWeights const &src, float weight) {
		w0 += weight * src.w0;
		w1 += weight * src.w1;
		w2 += weight * src.w2;
		w3 += weight * src.w3;
	}

	float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.f;
};

struct SubdivData {
	godot::PackedVector3Array vertex_array;
	godot::PackedVector3Array normal_array;
	godot::PackedVector2Array uv_array;
	godot::PackedInt32Array index_array;
	godot::PackedFloat32Array bones_array;
	godot::PackedFloat32Array weights_array;

	int32_t index_count = 0;
	int32_t face_count = 0;
	int32_t vertex_count = 0;
	int32_t uv_count = 0;
	int32_t bone_count = 0;
	int32_t weight_count = 0;
};
struct SurfaceVertexArrays {
	PackedVector3Array vertex_array;
	PackedVector3Array normal_array;
	PackedInt32Array index_array;
	PackedVector2Array uv_array;
	PackedInt32Array bones_array; //could be float or int array after docs
	PackedFloat32Array weights_array;
	SurfaceVertexArrays(Array p_mesh_arrays) {
		vertex_array = p_mesh_arrays[Mesh::ARRAY_VERTEX];
		normal_array = p_mesh_arrays[Mesh::ARRAY_NORMAL];
		index_array = p_mesh_arrays[Mesh::ARRAY_INDEX];
		uv_array = p_mesh_arrays[Mesh::ARRAY_TEX_UV];
		if (p_mesh_arrays[Mesh::ARRAY_BONES])
			bones_array = p_mesh_arrays[Mesh::ARRAY_BONES];

		if (p_mesh_arrays[Mesh::ARRAY_WEIGHTS])
			weights_array = p_mesh_arrays[Mesh::ARRAY_WEIGHTS];
	}
};

struct SurfaceData {
	Vector<int32_t> mesh_to_subdiv_index_map;
};

#endif