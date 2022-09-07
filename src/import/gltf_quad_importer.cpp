#include "gltf_quad_importer.hpp"

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "subdiv_mesh_instance_3d.hpp"

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/importer_mesh.hpp"
#include "godot_cpp/classes/importer_mesh_instance3d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

GLTFQuadImporter::GLTFQuadImporter() {
}

GLTFQuadImporter::~GLTFQuadImporter() {
}

void GLTFQuadImporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("convert_importer_meshinstance_to_quad"), &GLTFQuadImporter::convert_importer_meshinstance_to_quad);
	ClassDB::bind_method(D_METHOD("convert_meshinstance_to_quad", "MeshInstance3D"), &GLTFQuadImporter::convert_meshinstance_to_quad);
}

GLTFQuadImporter::SurfaceVertexArrays::SurfaceVertexArrays(const Array &p_mesh_arrays) {
	vertex_array = p_mesh_arrays[Mesh::ARRAY_VERTEX];
	normal_array = p_mesh_arrays[Mesh::ARRAY_NORMAL];
	index_array = p_mesh_arrays[Mesh::ARRAY_INDEX];
	uv_array = p_mesh_arrays[Mesh::ARRAY_TEX_UV];
	if (p_mesh_arrays[Mesh::ARRAY_BONES])
		bones_array = p_mesh_arrays[Mesh::ARRAY_BONES];

	if (p_mesh_arrays[Mesh::ARRAY_WEIGHTS])
		weights_array = p_mesh_arrays[Mesh::ARRAY_WEIGHTS];
}

GLTFQuadImporter::QuadSurfaceData GLTFQuadImporter::_remove_duplicate_vertices(const SurfaceVertexArrays &surface) {
	QuadSurfaceData quad_surface;
	int max_index = 0;
	HashMap<Vector3, int> original_verts;
	for (int vert_index = 0; vert_index < surface.index_array.size(); vert_index++) {
		int index = surface.index_array[vert_index];
		if (original_verts.has(surface.vertex_array[index])) {
			quad_surface.index_array.append(original_verts.get(surface.vertex_array[index]));
		} else {
			quad_surface.vertex_array.append(surface.vertex_array[index]);
			quad_surface.normal_array.append(surface.normal_array[index]);

			// weights and bones arrays to corresponding vertex (4 weights per vert)
			if (surface.weights_array.size() && surface.bones_array.size()) {
				for (int weight_index = index * 4; weight_index < (index + 1) * 4; weight_index++) {
					quad_surface.weights_array.append(surface.weights_array[weight_index]);
					quad_surface.bones_array.append(surface.bones_array[weight_index]);
				}
			}

			quad_surface.index_array.append(max_index);
			original_verts.insert(surface.vertex_array[index], max_index);

			max_index++;
		}

		// always
		quad_surface.uv_array.append(surface.uv_array[index]);
	}

	return quad_surface;
}

// Goes through index_array and always merges the 6 indices of 2 triangles to 1 quad (uv_array also updated)
void GLTFQuadImporter::_merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array) {
	ERR_FAIL_COND_MSG(index_array.size() % 6 != 0, "Mesh does not contain triangle amount to always merge two to one quad");
	PackedInt32Array quad_index_array;
	PackedVector2Array quad_uv_array;
	for (int i = 0; i < index_array.size(); i += 6) {
		// initalize unshared with verts from triangle 1
		PackedInt32Array unshared_verts;
		PackedInt32Array pos_unshared_verts; // array to keep track of index of index in index_array
		for (int t1 = i; t1 < i + 3; t1++) {
			unshared_verts.push_back(index_array[t1]);
			pos_unshared_verts.push_back(t1);
		}
		PackedInt32Array shared_verts;
		PackedInt32Array pos_shared_verts;
		// compare with triangle 2
		for (int t2 = i + 3; t2 < i + 6; t2++) {
			const int &index = index_array[t2]; // index from triangle 2
			int pos = unshared_verts.find(index, 0);
			if (pos != -1) {
				shared_verts.push_back(index);
				pos_shared_verts.push_back(t2);
				unshared_verts.remove_at(pos);
				pos_unshared_verts.remove_at(pos);
			} else {
				unshared_verts.push_back(index);
				pos_unshared_verts.push_back(t2);
			}
		}
		ERR_FAIL_COND_MSG(shared_verts.size() != 2, "Mesh not convertible to quad mesh");
		/*isn't always like this, but for recreating triangles later this is
		unshared[0]	 - 	shared[0]
			|				|
		shared[1]	 -	unshared[1]
		*/
		quad_index_array.append(unshared_verts[0]);
		quad_index_array.append(shared_verts[0]);
		quad_index_array.append(unshared_verts[1]);
		quad_index_array.append(shared_verts[1]);

		// uv's
		quad_uv_array.append(uv_array[pos_unshared_verts[0]]);
		quad_uv_array.append(uv_array[pos_shared_verts[0]]);
		quad_uv_array.append(uv_array[pos_unshared_verts[1]]);
		quad_uv_array.append(uv_array[pos_shared_verts[1]]);
	}
	index_array = quad_index_array;
	uv_array = quad_uv_array;
}

// tries to store uv's as compact as possible with an index array, an additional index array
// is needed for uv data, because the index array for the vertices connects faces while uv's
// can still be different for faces even when it's the same vertex, works similar to _remove_duplicate_vertices
PackedInt32Array GLTFQuadImporter::_generate_uv_index_array(PackedVector2Array &uv_array) {
	int max_index = 0;
	HashMap<Vector2, int> found_uvs;
	PackedInt32Array uv_index_array;
	PackedVector2Array packed_uv_array; // will overwrite the given uv_array before returning
	for (int uv_index = 0; uv_index < uv_array.size(); uv_index++) {
		Vector2 single_uv = uv_array[uv_index];
		if (found_uvs.has(single_uv)) {
			uv_index_array.append(found_uvs.get(single_uv));
		} else {
			packed_uv_array.append(single_uv);
			uv_index_array.append(max_index);
			found_uvs.insert(single_uv, max_index);
			max_index++;
		}
	}
	uv_array = packed_uv_array;
	return uv_index_array;
}

//same as remove duplicate vertices, just runs for every blend shape
Array GLTFQuadImporter::_generate_packed_blend_shapes(const Array &tri_blend_shapes, const PackedInt32Array &mesh_index_array, const PackedVector3Array &mesh_vertex_array) {
	int max_index = 0;
	HashMap<Vector3, int> original_verts;
	Vector<PackedVector3Array> packed_vertex_arrays;
	packed_vertex_arrays.resize(tri_blend_shapes.size());
	for (int vert_index = 0; vert_index < mesh_index_array.size(); vert_index++) {
		int index = mesh_index_array[vert_index];
		if (!original_verts.has(mesh_vertex_array[index])) {
			original_verts.insert(mesh_vertex_array[index], max_index);
			for (int blend_shape_idx = 0; blend_shape_idx < tri_blend_shapes.size(); blend_shape_idx++) { //TODO: possibly add normal and tangent here as well
				const Array &single_blend_shape_array = tri_blend_shapes[blend_shape_idx];
				const PackedVector3Array &tri_blend_vertex_array = single_blend_shape_array[Mesh::ARRAY_VERTEX];
				packed_vertex_arrays.write[blend_shape_idx].append(tri_blend_vertex_array[index] - mesh_vertex_array[index]);
			}
			max_index++;
		}
	}

	Array packed_blend_shape_array;
	packed_blend_shape_array.resize(tri_blend_shapes.size());
	for (int blend_shape_idx = 0; blend_shape_idx < tri_blend_shapes.size(); blend_shape_idx++) {
		Array single_blend_shape_array;
		single_blend_shape_array.resize(SubdivDataMesh::ARRAY_MAX);
		single_blend_shape_array[SubdivDataMesh::ARRAY_VERTEX] = packed_vertex_arrays.get(blend_shape_idx);
		packed_blend_shape_array[blend_shape_idx] = single_blend_shape_array;
	}
	return packed_blend_shape_array;
}

void GLTFQuadImporter::convert_meshinstance_to_quad(Object *p_meshinstance_object) {
	MeshInstance3D *p_meshinstance = Object::cast_to<MeshInstance3D>(p_meshinstance_object);
	Ref<ArrayMesh> p_mesh = p_meshinstance->get_mesh();
	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");

	SubdivDataMesh *quad_mesh = memnew(SubdivDataMesh);
	quad_mesh->set_name(p_mesh->get_name());
	// generate quad_mesh data
	for (int surface_index = 0; surface_index < p_mesh->get_surface_count(); surface_index++) {
		ERR_FAIL_COND_MSG(p_mesh->_surface_get_format(surface_index) & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS != 0, "Currently only 4 bone weights are supported. Conversion unsuccesful.");
		//convert actual mesh arrays to quad data
		Array p_arrays = p_mesh->surface_get_arrays(surface_index);
		Array quad_surface_arrays = generate_quad_mesh_arrays(SurfaceVertexArrays(p_arrays));

		//convert all blend shapes in the exact same way (BlendShapeArrays are also just an Array with the size of ARRAY_MAX and data offsets)
		Array blend_shape_arrays = p_mesh->surface_get_blend_shape_arrays(surface_index);
		Array quad_blend_shape_arrays;
		if (blend_shape_arrays.size()) {
			Array mesh_arrays = p_mesh->surface_get_arrays(surface_index);
			quad_blend_shape_arrays = _generate_packed_blend_shapes(blend_shape_arrays, mesh_arrays[Mesh::ARRAY_INDEX], mesh_arrays[Mesh::ARRAY_VERTEX]);
		} //TODO: blend shape data also contains ARRAY_NORMAL and ARRAY_TANGENT, considering normals get generated for subdivisions won't get added for now

		ERR_FAIL_COND(!quad_surface_arrays.size());
		quad_mesh->add_surface(quad_surface_arrays, quad_blend_shape_arrays, p_mesh->surface_get_material(surface_index), p_mesh->surface_get_name(surface_index));
	}
	//actually add blendshapes to data
	for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
		quad_mesh->_add_blend_shape_name(p_mesh->get_blend_shape_name(blend_shape_idx));
	}

	SubdivMeshInstance3D *quad_mesh_instance = memnew(SubdivMeshInstance3D);
	// skin data and such are not changed and will just be applied to generated helper triangle mesh later.
	quad_mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
	if (!p_meshinstance->get_skin().is_null()) {
		quad_mesh_instance->set_skin(p_meshinstance->get_skin());
	}

	StringName quad_mesh_instance_name = p_meshinstance->get_name();
	quad_mesh_instance->set_mesh(quad_mesh);

	// replace importermeshinstance in scene
	p_meshinstance->replace_by(quad_mesh_instance, false);
	quad_mesh_instance->set_name(quad_mesh_instance_name);
	p_meshinstance->queue_free();
}

void GLTFQuadImporter::convert_importer_meshinstance_to_quad(Object *p_meshinstance_object) {
	ImporterMeshInstance3D *p_meshinstance = Object::cast_to<ImporterMeshInstance3D>(p_meshinstance_object);
	Ref<ImporterMesh> p_mesh = p_meshinstance->get_mesh();
	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");

	SubdivDataMesh *quad_mesh = memnew(SubdivDataMesh);
	quad_mesh->set_name(p_mesh->get_name());
	// generate quad_mesh data
	for (int surface_index = 0; surface_index < p_mesh->get_surface_count(); surface_index++) {
		ERR_FAIL_COND_MSG(p_mesh->get_surface_format(surface_index) & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS != 0, "Currently only 4 bone weights are supported. Conversion unsuccesful.");
		//convert actual mesh data to quad
		Array p_arrays = p_mesh->get_surface_arrays(surface_index);
		SurfaceVertexArrays surface = SurfaceVertexArrays(p_arrays);
		Array quad_surface_arrays = generate_quad_mesh_arrays(SurfaceVertexArrays(p_arrays));

		//convert all blend shapes in the exact same way (BlendShapeArrays are also just an Array with the size of ARRAY_MAX and data offsets)
		p_arrays = p_mesh->get_surface_arrays(surface_index);
		Array blend_shape_arrays;
		Array quad_blend_shape_arrays;
		for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
			blend_shape_arrays.push_back(p_mesh->get_surface_blend_shape_arrays(surface_index, blend_shape_idx));
		}
		if (blend_shape_arrays.size()) {
			quad_blend_shape_arrays = _generate_packed_blend_shapes(blend_shape_arrays, p_arrays[Mesh::ARRAY_INDEX], p_arrays[Mesh::ARRAY_VERTEX]);
		}

		ERR_FAIL_COND(!quad_surface_arrays.size());
		quad_mesh->add_surface(quad_surface_arrays, quad_blend_shape_arrays, p_mesh->get_surface_material(surface_index), p_mesh->get_surface_name(surface_index));
	}
	//actually add blendshapes to data
	for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
		quad_mesh->_add_blend_shape_name(p_mesh->get_blend_shape_name(blend_shape_idx));
	}

	SubdivMeshInstance3D *quad_mesh_instance = memnew(SubdivMeshInstance3D);
	// skin data and such are not changed and will just be applied to generated helper triangle mesh later.
	quad_mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
	if (!p_meshinstance->get_skin().is_null()) {
		quad_mesh_instance->set_skin(p_meshinstance->get_skin());
	}

	StringName quad_mesh_instance_name = p_meshinstance->get_name();
	quad_mesh_instance->set_mesh(quad_mesh);

	// replace importermeshinstance in scene
	p_meshinstance->replace_by(quad_mesh_instance, false);
	quad_mesh_instance->set_name(quad_mesh_instance_name);
	p_meshinstance->queue_free();
}

Array GLTFQuadImporter::generate_quad_mesh_arrays(const SurfaceVertexArrays &surface) {
	QuadSurfaceData quad_surface = _remove_duplicate_vertices(surface);
	_merge_to_quads(quad_surface.index_array, quad_surface.uv_array);
	ERR_FAIL_COND_V(quad_surface.index_array.size() / 4 != surface.index_array.size() / 6, Array());

	Array quad_surface_arrays;
	quad_surface_arrays.resize(SubdivDataMesh::ARRAY_MAX);
	quad_surface_arrays[SubdivDataMesh::ARRAY_VERTEX] = quad_surface.vertex_array;
	quad_surface_arrays[SubdivDataMesh::ARRAY_NORMAL] = quad_surface.normal_array;
	// quad_surface_arrays[Mesh::ARRAY_TANGENT]
	quad_surface_arrays[SubdivDataMesh::ARRAY_BONES] = quad_surface.bones_array; // TODO: docs say bones array can also be floats, might cause issues
	quad_surface_arrays[SubdivDataMesh::ARRAY_WEIGHTS] = quad_surface.weights_array;
	quad_surface_arrays[SubdivDataMesh::ARRAY_INDEX] = quad_surface.index_array;

	PackedInt32Array uv_index_array = _generate_uv_index_array(quad_surface.uv_array);
	quad_surface_arrays[SubdivDataMesh::ARRAY_TEX_UV] = quad_surface.uv_array;
	quad_surface_arrays[SubdivDataMesh::ARRAY_UV_INDEX] = uv_index_array;
	return quad_surface_arrays;
}
