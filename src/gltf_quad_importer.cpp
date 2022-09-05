#include "gltf_quad_importer.hpp"

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "quad_mesh_instance_3d.hpp"

#include "godot_cpp/classes/importer_mesh.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

GLTFQuadImporter::GLTFQuadImporter() {
}

GLTFQuadImporter::~GLTFQuadImporter() {
}

void GLTFQuadImporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("convert_importer_meshinstance_to_quad"), &GLTFQuadImporter::convert_importer_meshinstance_to_quad);
	// ClassDB::bind_method(D_METHOD("convert_meshinstance_to_quad", "MeshInstance3D"), &GLTFQuadImporter::convert_meshinstance_to_quad);
}

//TODO: blendshape conversion, also needs to happen here
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

			//weights and bones arrays to corresponding vertex (4 weights per vert)
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

		//always
		quad_surface.uv_array.append(surface.uv_array[index]);
	}

	return quad_surface;
}

//Goes through index_array and always merges the 6 indices of 2 triangles to 1 quad (uv_array also updated)
void GLTFQuadImporter::_merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array) {
	ERR_FAIL_COND_MSG(index_array.size() % 6 != 0, "Mesh does not contain triangle amount to always merge two to one quad");
	PackedInt32Array quad_index_array;
	PackedVector2Array quad_uv_array;
	for (int i = 0; i < index_array.size(); i += 6) {
		//initalize unshared with verts from triangle 1
		PackedInt32Array unshared_verts;
		PackedInt32Array pos_unshared_verts; //array to keep track of index of index in index_array
		for (int t1 = i; t1 < i + 3; t1++) {
			unshared_verts.push_back(index_array[t1]);
			pos_unshared_verts.push_back(t1);
		}
		PackedInt32Array shared_verts;
		PackedInt32Array pos_shared_verts;
		//compare with triangle 2
		for (int t2 = i + 3; t2 < i + 6; t2++) {
			const int &index = index_array[t2]; //index from triangle 2
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

		//uv's
		quad_uv_array.append(uv_array[pos_unshared_verts[0]]);
		quad_uv_array.append(uv_array[pos_shared_verts[0]]);
		quad_uv_array.append(uv_array[pos_unshared_verts[1]]);
		quad_uv_array.append(uv_array[pos_shared_verts[1]]);
	}
	index_array = quad_index_array;
	uv_array = quad_uv_array;
}

//tries to store uv's as compact as possible with an index array, an additional index array
//is needed for uv data, because the index array for the vertices connects faces while uv's
//can still be different for faces even when it's the same vertex, works similar to _remove_duplicate_vertices
PackedInt32Array GLTFQuadImporter::_generate_uv_index_array(PackedVector2Array &uv_array) {
	int max_index = 0;
	HashMap<Vector2, int> found_uvs;
	PackedInt32Array uv_index_array;
	PackedVector2Array packed_uv_array; //will overwrite the given uv_array before returning
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

// void GLTFQuadImporter::convert_meshinstance_to_quad(Ref<MeshInstance3D> &p_meshinstance) {
// 	Ref<Mesh> p_mesh = p_meshinstance->get_mesh();
// 	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");
// 	ImporterQuadMesh quad_mesh;
// 	SurfaceVertexArrays surface = SurfaceVertexArrays(p_mesh->surface_get_arrays(0));
// 	SubdivData quad_surface = _remove_duplicate_vertices(surface);
// 	_merge_to_quads(quad_surface.index_array);
// }

void GLTFQuadImporter::convert_importer_meshinstance_to_quad(Object *p_meshinstance_object) {
	ImporterMeshInstance3D *p_meshinstance = Object::cast_to<ImporterMeshInstance3D>(p_meshinstance_object);
	Ref<ImporterMesh> p_mesh = p_meshinstance->get_mesh();
	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");

	ImporterQuadMesh *quad_mesh = memnew(ImporterQuadMesh);
	quad_mesh->set_name(p_mesh->get_name());
	//generate quad_mesh data
	for (int surface_index = 0; surface_index < p_mesh->get_surface_count(); surface_index++) {
		ERR_FAIL_COND_MSG(p_mesh->get_surface_format(surface_index) & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS != 0, "Currently only 4 bone weights are supported. Conversion unsuccesful.");
		Array p_arrays = p_mesh->get_surface_arrays(0);
		SurfaceVertexArrays surface = SurfaceVertexArrays(p_arrays);
		QuadSurfaceData quad_surface = _remove_duplicate_vertices(surface);
		_merge_to_quads(quad_surface.index_array, quad_surface.uv_array);
		ERR_FAIL_COND(quad_surface.index_array.size() / 4 != surface.index_array.size() / 6);

		Array subdiv_surface_array;
		subdiv_surface_array.resize(ImporterQuadMesh::ARRAY_MAX);
		subdiv_surface_array[ImporterQuadMesh::ARRAY_VERTEX] = quad_surface.vertex_array;
		subdiv_surface_array[ImporterQuadMesh::ARRAY_NORMAL] = quad_surface.normal_array;
		//subdiv_surface_array[Mesh::ARRAY_TANGENT]
		subdiv_surface_array[ImporterQuadMesh::ARRAY_BONES] = quad_surface.bones_array; //TODO: docs say bones array can also be floats, might cause issues
		subdiv_surface_array[ImporterQuadMesh::ARRAY_WEIGHTS] = quad_surface.weights_array;
		subdiv_surface_array[ImporterQuadMesh::ARRAY_INDEX] = quad_surface.index_array;

		PackedInt32Array uv_index_array = _generate_uv_index_array(quad_surface.uv_array);
		subdiv_surface_array[ImporterQuadMesh::ARRAY_TEX_UV] = quad_surface.uv_array;
		subdiv_surface_array[ImporterQuadMesh::ARRAY_UV_INDEX] = uv_index_array;
		//TODO: add blendshapes
		quad_mesh->add_surface(subdiv_surface_array, Array(), p_mesh->get_surface_material(surface_index), p_mesh->get_surface_name(surface_index));
	}

	QuadMeshInstance3D *quad_mesh_instance = memnew(QuadMeshInstance3D);
	//skin data and such are not changed and will just be applied to generated helper triangle mesh later.
	quad_mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
	quad_mesh_instance->set_skin(p_meshinstance->get_skin());

	StringName quad_mesh_instance_name = p_meshinstance->get_name();
	quad_mesh_instance->set_mesh(quad_mesh);

	//replace importermeshinstance in scene
	p_meshinstance->replace_by(quad_mesh_instance, false);
	quad_mesh_instance->set_name(quad_mesh_instance_name);
	p_meshinstance->queue_free();
}

void GLTFQuadImporter::print_index_array(const SurfaceVertexArrays &surface) {
	UtilityFunctions::print(surface.index_array);
}
