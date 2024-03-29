#include "local_mesh.h"

#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/core/class_db.hpp"

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/builtin_types.hpp"

#include "godot_cpp/classes/material.hpp"

#include "godot_cpp/classes/mesh.hpp"

void LocalMesh::clear_surfaces() {
	RenderingServer::get_singleton()->mesh_clear(local_mesh);
	mesh_surface_offsets.clear();
	num_surfaces = 0;
}

void LocalMesh::add_surface(const Array &p_arrays, const Dictionary &p_lods, const Ref<Material> &p_material,
		const String &p_name, int32_t p_format) {
	RenderingServer *rendering_server = RenderingServer::get_singleton();

	//add mesh and set material
	rendering_server->mesh_add_surface_from_arrays(local_mesh, RenderingServer::PRIMITIVE_TRIANGLES, p_arrays, Array(), p_lods, p_format);
	rendering_server->mesh_surface_set_material(local_mesh, num_surfaces, p_material.is_null() ? RID() : p_material->get_rid());

	//get vertex stride offsets, needed for updating vertices, normals and tangents in update_surface_vertices
	const PackedVector3Array &vertex_array = p_arrays[Mesh::ARRAY_VERTEX];

	Vector<uint32_t> offsets;
	offsets.resize(Mesh::ARRAY_MAX);
	offsets.write[Mesh::ARRAY_VERTEX] = rendering_server->mesh_surface_get_format_offset(p_format, vertex_array.size(), Mesh::ARRAY_VERTEX);
	offsets.write[Mesh::ARRAY_NORMAL] = rendering_server->mesh_surface_get_format_offset(p_format, vertex_array.size(), Mesh::ARRAY_NORMAL);
	offsets.write[Mesh::ARRAY_TANGENT] = rendering_server->mesh_surface_get_format_offset(p_format, vertex_array.size(), Mesh::ARRAY_TANGENT);
	mesh_surface_offsets.append(offsets);

	vertex_strides.append(rendering_server->mesh_surface_get_format_vertex_stride(p_format, vertex_array.size()));
	num_surfaces++;
}

void LocalMesh::update_surface_vertices(int surface_idx, const Array &p_arrays) {
	ERR_FAIL_COND(p_arrays.size() != Mesh::ARRAY_MAX);
	ERR_FAIL_INDEX(surface_idx, num_surfaces);

	// update vertices
	//Vector<uint8_t> vertex_data; // Vertex, Normal, Tangent (change with skinning, blendshape).
	PackedByteArray vertex_buffer = RenderingServer::get_singleton()->mesh_get_surface(local_mesh, surface_idx)["vertex_data"];
	uint8_t *vertex_write_buffer = vertex_buffer.ptrw();

	uint32_t vertex_stride = vertex_strides.get(surface_idx); //vector3 size

	const PackedVector3Array &vertex_array = p_arrays[Mesh::ARRAY_VERTEX];
	const PackedVector3Array &normal_array = p_arrays[Mesh::ARRAY_NORMAL];
	const PackedFloat32Array &tangent_array = p_arrays[Mesh::ARRAY_TANGENT];

	bool has_normals = p_arrays[Mesh::ARRAY_NORMAL] && normal_array.size() > 0;
	bool has_tangents = p_arrays[Mesh::ARRAY_TANGENT] && tangent_array.size() > 0;

	const Vector<uint32_t> &vertex_stride_offsets = mesh_surface_offsets[surface_idx];

	//actually update
	for (int vertex_index = 0; vertex_index < vertex_array.size(); vertex_index++) {
		//normal and tangent get compressed here, code taken from Sprite3D implementation
		// uv's and positions/vertex do not get compressed.
		memcpy(&vertex_write_buffer[vertex_index * vertex_stride + vertex_stride_offsets[Mesh::ARRAY_VERTEX]], &vertex_array[vertex_index], sizeof(float) * 3);
		if (has_normals) {
			Vector3 normal = normal_array[vertex_index];

			uint32_t v_normal;
			{ //code to compress normal
				Vector2 res = normal.octahedron_encode();
				uint32_t value = 0;
				value |= (uint16_t)CLAMP(res.x * 65535, 0, 65535);
				value |= (uint16_t)CLAMP(res.y * 65535, 0, 65535) << 16;

				v_normal = value;
			}

			memcpy(&vertex_write_buffer[vertex_index * vertex_stride + vertex_stride_offsets[Mesh::ARRAY_NORMAL]], &v_normal, 4);
		}

		if (has_tangents) {
			int tangent_index = vertex_index * 4;
			Plane tangent = tangent = Plane(tangent_array[tangent_index], tangent_array[tangent_index + 1], tangent_array[tangent_index + 2], tangent_array[tangent_index + 3]);
			uint32_t v_tangent;
			{ //code to compress tangent
				Plane t = tangent;
				Vector2 res = t.normal.octahedron_tangent_encode(t.d);
				uint32_t value = 0;
				value |= (uint16_t)CLAMP(res.x * 65535, 0, 65535);
				value |= (uint16_t)CLAMP(res.y * 65535, 0, 65535) << 16;

				v_tangent = value;
			}

			memcpy(&vertex_write_buffer[vertex_index * vertex_stride + vertex_stride_offsets[Mesh::ARRAY_TANGENT]], &v_tangent, 4);
		}
	}

	RenderingServer::get_singleton()->mesh_surface_update_vertex_region(local_mesh, surface_idx, 0, vertex_buffer);
}

RID LocalMesh::get_rid() const {
	return local_mesh;
}

void LocalMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("clear_surfaces"), &LocalMesh::clear_surfaces);
	ClassDB::bind_method(D_METHOD("add_surface", "p_arrays", "p_lods", "p_material", "p_name", "p_format"), &LocalMesh::add_surface);
	ClassDB::bind_method(D_METHOD("update_surface_vertices", "surface_idx", "p_arrays"), &LocalMesh::update_surface_vertices);
	ClassDB::bind_method(D_METHOD("get_rid"), &LocalMesh::get_rid);
}

LocalMesh::LocalMesh() {
	local_mesh = RenderingServer::get_singleton()->mesh_create();
}

LocalMesh::~LocalMesh() {
	if (local_mesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(local_mesh);
	}

	local_mesh = RID();
}
