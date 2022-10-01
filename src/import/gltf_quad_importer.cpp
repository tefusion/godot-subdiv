#include "gltf_quad_importer.hpp"

#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "nodes/subdiv_mesh_instance_3d.hpp"

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/importer_mesh.hpp"
#include "godot_cpp/classes/importer_mesh_instance3d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/skeleton3d.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/baked_subdiv_mesh.hpp"

GLTFQuadImporter::GLTFQuadImporter() {
}

GLTFQuadImporter::~GLTFQuadImporter() {
}

void GLTFQuadImporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("convert_importer_meshinstance_to_quad"), &GLTFQuadImporter::convert_importer_meshinstance_to_quad);
	ClassDB::bind_method(D_METHOD("convert_meshinstance_to_quad", "MeshInstance3D"), &GLTFQuadImporter::convert_meshinstance_to_quad);
	BIND_ENUM_CONSTANT(SUBDIV_MESHINSTANCE);
	BIND_ENUM_CONSTANT(BAKED_SUBDIV_MESH);
	BIND_ENUM_CONSTANT(ARRAY_MESH);
}

GLTFQuadImporter::SurfaceVertexArrays::SurfaceVertexArrays(const Array &p_mesh_arrays) {
	vertex_array = p_mesh_arrays[Mesh::ARRAY_VERTEX];
	normal_array = p_mesh_arrays[Mesh::ARRAY_NORMAL];
	index_array = p_mesh_arrays[Mesh::ARRAY_INDEX];
	if (p_mesh_arrays[Mesh::ARRAY_TEX_UV]) {
		uv_array = p_mesh_arrays[Mesh::ARRAY_TEX_UV];
	}
	if (p_mesh_arrays[Mesh::ARRAY_BONES])
		bones_array = p_mesh_arrays[Mesh::ARRAY_BONES];

	if (p_mesh_arrays[Mesh::ARRAY_WEIGHTS])
		weights_array = p_mesh_arrays[Mesh::ARRAY_WEIGHTS];
}

void GLTFQuadImporter::convert_meshinstance_to_quad(Object *p_meshinstance_object) {
	MeshInstance3D *p_meshinstance = Object::cast_to<MeshInstance3D>(p_meshinstance_object);
	Ref<ArrayMesh> p_mesh = p_meshinstance->get_mesh();
	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");

	TopologyDataMesh *quad_mesh = memnew(TopologyDataMesh);
	quad_mesh->set_name(p_mesh->get_name());
	// generate quad_mesh data
	for (int surface_index = 0; surface_index < p_mesh->get_surface_count(); surface_index++) {
		//convert actual mesh arrays to quad data
		Array p_arrays = p_mesh->surface_get_arrays(surface_index);
		int32_t format = p_mesh->surface_get_format(surface_index);
		Array surface_arrays;
		TopologyDataMesh::TopologyType topology_type = _generate_topology_surface_arrays(SurfaceVertexArrays(p_arrays), format, surface_arrays);

		//convert all blend shapes in the exact same way (BlendShapeArrays are also just an Array with the size of ARRAY_MAX and data offsets)
		Array blend_shape_arrays = p_mesh->surface_get_blend_shape_arrays(surface_index);
		Array quad_blend_shape_arrays;
		if (!blend_shape_arrays.is_empty()) {
			quad_blend_shape_arrays = _generate_packed_blend_shapes(blend_shape_arrays, p_arrays[Mesh::ARRAY_INDEX], p_arrays[Mesh::ARRAY_VERTEX]);
		} //TODO: blend shape data also contains ARRAY_NORMAL and ARRAY_TANGENT, considering normals get generated for subdivisions won't get added for now

		ERR_FAIL_COND(!surface_arrays.size());
		quad_mesh->add_surface(surface_arrays, quad_blend_shape_arrays, p_mesh->surface_get_material(surface_index),
				p_mesh->surface_get_name(surface_index), format, topology_type);
	}
	//actually add blendshapes to data
	for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
		quad_mesh->add_blend_shape_name(p_mesh->get_blend_shape_name(blend_shape_idx));
	}

	SubdivMeshInstance3D *subdiv_mesh_instance = memnew(SubdivMeshInstance3D);
	// skin data and such are not changed and will just be applied to generated helper triangle mesh later.
	subdiv_mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
	if (!p_meshinstance->get_skin().is_null()) {
		subdiv_mesh_instance->set_skin(p_meshinstance->get_skin());
	}
	subdiv_mesh_instance->set_transform(p_meshinstance->get_transform());
	StringName quad_mesh_instance_name = p_meshinstance->get_name();
	subdiv_mesh_instance->set_mesh(quad_mesh);

	// replace importermeshinstance in scene
	p_meshinstance->replace_by(subdiv_mesh_instance, false);
	subdiv_mesh_instance->set_name(quad_mesh_instance_name);
	p_meshinstance->queue_free();
}

void GLTFQuadImporter::convert_importer_meshinstance_to_quad(Object *p_meshinstance_object, ImportMode import_mode, int32_t subdiv_level) {
	ImporterMeshInstance3D *p_meshinstance = Object::cast_to<ImporterMeshInstance3D>(p_meshinstance_object);
	Ref<ImporterMesh> p_mesh = p_meshinstance->get_mesh();
	ERR_FAIL_COND_MSG(p_mesh.is_null(), "Mesh is null");

	//if arraymesh and subdiv_level 0 just normal import
	if (import_mode == ImportMode::ARRAY_MESH && subdiv_level == 0) {
		MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
		Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
		array_mesh = p_mesh->get_mesh(array_mesh);
		mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
		if (!p_meshinstance->get_skin().is_null()) {
			mesh_instance->set_skin(p_meshinstance->get_skin());
		}
		mesh_instance->set_transform(p_meshinstance->get_transform());
		StringName mesh_instance_name = p_meshinstance->get_name();
		mesh_instance->set_mesh(array_mesh);
		p_meshinstance->replace_by(mesh_instance, false);
		mesh_instance->set_name(mesh_instance_name);
		return;
	}

	TopologyDataMesh *topology_data_mesh = memnew(TopologyDataMesh);
	topology_data_mesh->set_name(p_mesh->get_name());
	// generate quad_mesh data
	for (int surface_index = 0; surface_index < p_mesh->get_surface_count(); surface_index++) {
		//convert actual mesh data to quad
		Array p_arrays = p_mesh->get_surface_arrays(surface_index);
		SurfaceVertexArrays surface = SurfaceVertexArrays(p_arrays);
		int32_t format = generate_fake_format(p_arrays); //importermesh surface_get_format just returns flags
		Array surface_arrays;
		TopologyDataMesh::TopologyType topology_type = _generate_topology_surface_arrays(SurfaceVertexArrays(p_arrays), format, surface_arrays);

		//convert all blend shapes in the exact same way (BlendShapeArrays are also just an Array with the size of ARRAY_MAX and data offsets)
		Array blend_shape_arrays;
		Array quad_blend_shape_arrays;
		for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
			blend_shape_arrays.push_back(p_mesh->get_surface_blend_shape_arrays(surface_index, blend_shape_idx));
		}
		if (!blend_shape_arrays.is_empty()) {
			quad_blend_shape_arrays = _generate_packed_blend_shapes(blend_shape_arrays, p_arrays[Mesh::ARRAY_INDEX], p_arrays[Mesh::ARRAY_VERTEX]);
		}

		ERR_FAIL_COND(!surface_arrays.size());
		topology_data_mesh->add_surface(surface_arrays, quad_blend_shape_arrays,
				p_mesh->get_surface_material(surface_index), p_mesh->get_surface_name(surface_index), format, topology_type);
	}
	//actually add blendshapes to data
	for (int blend_shape_idx = 0; blend_shape_idx < p_mesh->get_blend_shape_count(); blend_shape_idx++) {
		topology_data_mesh->add_blend_shape_name(p_mesh->get_blend_shape_name(blend_shape_idx));
	}

	StringName mesh_instance_name = p_meshinstance->get_name();
	//creates subdiv mesh instance with topologydatamesh
	if (import_mode == ImportMode::SUBDIV_MESHINSTANCE) {
		SubdivMeshInstance3D *subdiv_mesh_instance = memnew(SubdivMeshInstance3D);
		// skin data and such are not changed and will just be applied to generated helper triangle mesh later.
		subdiv_mesh_instance->set_skeleton_path(p_meshinstance->get_skeleton_path());
		if (!p_meshinstance->get_skin().is_null()) {
			subdiv_mesh_instance->set_skin(p_meshinstance->get_skin());
		}
		subdiv_mesh_instance->set_transform(p_meshinstance->get_transform());
		subdiv_mesh_instance->set_mesh(topology_data_mesh);

		subdiv_mesh_instance->set_subdiv_level(subdiv_level);

		// replace importermeshinstance in scene
		p_meshinstance->replace_by(subdiv_mesh_instance, false);
		subdiv_mesh_instance->set_name(mesh_instance_name);
		p_meshinstance->queue_free();
	} else if (import_mode == ImportMode::ARRAY_MESH) {
		Ref<ImporterMesh> subdiv_importer_mesh;
		subdiv_importer_mesh.instantiate();
		{
			Ref<BakedSubdivMesh> subdiv_mesh;
			subdiv_mesh.instantiate();
			subdiv_mesh->set_data_mesh(topology_data_mesh);
			subdiv_mesh->set_subdiv_level(subdiv_level);
			//for (int64_t blend_shape_idx = 0; blend_shape_idx < subdiv_mesh->_get_blend_shape_count(); blend_shape_idx++) {
			//	StringName shape_name = subdiv_mesh->_get_blend_shape_name(blend_shape_idx);
			//	subdiv_importer_mesh->add_blend_shape(shape_name);
			//}
			for (int64_t surface_i = 0; surface_i < subdiv_mesh->_get_surface_count(); surface_i++) {
				Ref<Material> material = subdiv_mesh->_surface_get_material(surface_i);
				Array blend_shape_arrays;
				//for (int64_t blend_shape_i = 0; blend_shape_i < subdiv_mesh->_get_blend_shape_count(); blend_shape_i++) {
				//	blend_shape_arrays.push_back(subdiv_mesh->_surface_get_blend_shape_arrays(blend_shape_i));
				//}
				subdiv_importer_mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES, subdiv_mesh->_surface_get_arrays(surface_i), blend_shape_arrays, Dictionary(), material, material->get_name(), subdiv_mesh->_surface_get_format(surface_i));
			}
		}

		Array skin_pose_transform_array;
		{
			const Ref<Skin> skin = p_meshinstance->get_skin();
			if (skin.is_valid()) {
				NodePath skeleton_path = p_meshinstance->get_skeleton_path();
				const Node *node = p_meshinstance->get_node_or_null(skeleton_path);
				const Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
				if (skeleton) {
					int bind_count = skin->get_bind_count();

					for (int i = 0; i < bind_count; i++) {
						Transform3D bind_pose = skin->get_bind_pose(i);
						String bind_name = skin->get_bind_name(i);

						int bone_idx = bind_name.is_empty() ? skin->get_bind_bone(i) : skeleton->find_bone(bind_name);
						ERR_FAIL_COND(bone_idx >= skeleton->get_bone_count());
						Transform3D bp_global_rest;
						if (bone_idx >= 0) {
							bp_global_rest = skeleton->get_bone_global_pose(bone_idx);
						} else {
							bp_global_rest = skeleton->get_bone_global_pose(i);
						}

						skin_pose_transform_array.push_back(bp_global_rest * bind_pose);
					}
				}
			}
		}
		subdiv_importer_mesh->generate_lods(UtilityFunctions::deg_to_rad(25), UtilityFunctions::deg_to_rad(60), skin_pose_transform_array);
		Ref<ImporterMesh> base_mesh;
		base_mesh.instantiate();
		// Does this initiation fix a crash?
		Ref<ArrayMesh> array_mesh = subdiv_importer_mesh->get_mesh(base_mesh);
		p_meshinstance->set_mesh(array_mesh);
	} else {
		ERR_PRINT("Import mode doesn't exist");
	}
}

TopologyDataMesh::TopologyType GLTFQuadImporter::_generate_topology_surface_arrays(const SurfaceVertexArrays &surface, int32_t format, Array &surface_arrays) {
	ERR_FAIL_COND_V(!(format & Mesh::ARRAY_FORMAT_INDEX), TopologyDataMesh::TopologyType::QUAD);
	QuadSurfaceData quad_surface = _remove_duplicate_vertices(surface, format);
	bool is_quad = _merge_to_quads(quad_surface.index_array, quad_surface.uv_array, format);
	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;

	surface_arrays.resize(TopologyDataMesh::ARRAY_MAX);
	surface_arrays[TopologyDataMesh::ARRAY_VERTEX] = quad_surface.vertex_array;
	surface_arrays[TopologyDataMesh::ARRAY_NORMAL] = quad_surface.normal_array;
	// surface_arrays[Mesh::ARRAY_TANGENT]
	surface_arrays[TopologyDataMesh::ARRAY_BONES] = quad_surface.bones_array; // TODO: docs say bones array can also be floats, might cause issues
	surface_arrays[TopologyDataMesh::ARRAY_WEIGHTS] = quad_surface.weights_array;
	surface_arrays[TopologyDataMesh::ARRAY_INDEX] = quad_surface.index_array;
	if (has_uv) {
		PackedInt32Array uv_index_array = _generate_uv_index_array(quad_surface.uv_array);
		surface_arrays[TopologyDataMesh::ARRAY_TEX_UV] = quad_surface.uv_array;
		surface_arrays[TopologyDataMesh::ARRAY_UV_INDEX] = uv_index_array;
	} else { //this is to avoid issues with casting null to Array when using these as reference
		surface_arrays[TopologyDataMesh::ARRAY_TEX_UV] = PackedVector2Array();
		surface_arrays[TopologyDataMesh::ARRAY_UV_INDEX] = PackedInt32Array();
	}

	if (is_quad) {
		return TopologyDataMesh::TopologyType::QUAD;
	} else {
		return TopologyDataMesh::TopologyType::TRIANGLE;
	}
}

GLTFQuadImporter::QuadSurfaceData GLTFQuadImporter::_remove_duplicate_vertices(const SurfaceVertexArrays &surface, int32_t format) {
	//these booleans decide what data gets stored, could be used directly with format, but I think this is more readable
	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;
	bool has_skinning = (format & Mesh::ARRAY_FORMAT_BONES) && (format & Mesh::ARRAY_FORMAT_WEIGHTS);
	bool double_bone_weights = format & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	bool has_normals = format & Mesh::ARRAY_FORMAT_NORMAL;
	//TODO: maybe add tangents, uv2 here too, considering that data is lost after subdivisions not that urgent

	QuadSurfaceData quad_surface;
	int max_index = 0;
	HashMap<Vector3, int> original_verts;
	for (int vert_index = 0; vert_index < surface.index_array.size(); vert_index++) {
		int index = surface.index_array[vert_index];
		if (original_verts.has(surface.vertex_array[index])) {
			quad_surface.index_array.append(original_verts.get(surface.vertex_array[index]));
		} else {
			quad_surface.vertex_array.append(surface.vertex_array[index]);
			if (has_normals) {
				quad_surface.normal_array.append(surface.normal_array[index]);
			}

			// weights and bones arrays to corresponding vertex (4 weights per vert)
			if (has_skinning) {
				int bone_weights_per_vert = double_bone_weights ? 8 : 4;
				for (int weight_index = index * bone_weights_per_vert; weight_index < (index + 1) * bone_weights_per_vert; weight_index++) {
					quad_surface.weights_array.append(surface.weights_array[weight_index]);
					quad_surface.bones_array.append(surface.bones_array[weight_index]);
				}
			}

			quad_surface.index_array.append(max_index);

			original_verts.insert(surface.vertex_array[index], max_index);

			max_index++;
		}

		// always if uv's exist
		if (has_uv) {
			quad_surface.uv_array.append(surface.uv_array[index]);
		}
	}

	return quad_surface;
}

// Goes through index_array and always merges the 6 indices of 2 triangles to 1 quad (uv_array also updated)
// returns boolean if conversion to quad was successful
bool GLTFQuadImporter::_merge_to_quads(PackedInt32Array &index_array, PackedVector2Array &uv_array, int32_t format) {
	if (index_array.size() % 6 != 0) {
		return false;
	}

	bool has_uv = format & Mesh::ARRAY_FORMAT_TEX_UV;

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
		if (shared_verts.size() != 2) {
			return false;
		}

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
		if (has_uv) {
			quad_uv_array.append(uv_array[pos_unshared_verts[0]]);
			quad_uv_array.append(uv_array[pos_shared_verts[0]]);
			quad_uv_array.append(uv_array[pos_unshared_verts[1]]);
			quad_uv_array.append(uv_array[pos_shared_verts[1]]);
		}
	}
	index_array = quad_index_array;
	uv_array = quad_uv_array;
	return true;
}

// tries to store uv's as compact as possible with an index array, an additional index array
// is needed for uv data, because the index array for the vertices connects faces while uv's
// can still be different for faces even when it's the same vertex, works similar to _remove_duplicate_vertices
PackedInt32Array GLTFQuadImporter::_generate_uv_index_array(PackedVector2Array &uv_array) {
	ERR_FAIL_COND_V(uv_array.is_empty(), PackedInt32Array());
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
Array GLTFQuadImporter::_generate_packed_blend_shapes(const Array &tri_blend_shapes, const PackedInt32Array &mesh_index_array,
		const PackedVector3Array &mesh_vertex_array) {
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
		single_blend_shape_array.resize(TopologyDataMesh::ARRAY_MAX);
		single_blend_shape_array[TopologyDataMesh::ARRAY_VERTEX] = packed_vertex_arrays.get(blend_shape_idx);
		packed_blend_shape_array[blend_shape_idx] = single_blend_shape_array;
	}
	return packed_blend_shape_array;
}

//TODO: maybe godot has a function for this so there's no need for this mess to get the format
//the importer mesh format is a lie and just gives a flag
int32_t GLTFQuadImporter::generate_fake_format(const Array &arrays) const {
	int32_t format = 0;
	if (arrays[Mesh::ARRAY_VERTEX])
		format |= Mesh::ARRAY_FORMAT_VERTEX;
	if (arrays[Mesh::ARRAY_NORMAL])
		format |= Mesh::ARRAY_FORMAT_NORMAL;
	if (arrays[Mesh::ARRAY_TANGENT])
		format |= Mesh::ARRAY_FORMAT_TANGENT;
	if (arrays[Mesh::ARRAY_COLOR])
		format |= Mesh::ARRAY_FORMAT_COLOR;
	if (arrays[Mesh::ARRAY_TEX_UV])
		format |= Mesh::ARRAY_FORMAT_TEX_UV;
	if (arrays[Mesh::ARRAY_TEX_UV2])
		format |= Mesh::ARRAY_FORMAT_TEX_UV2;
	if (arrays[Mesh::ARRAY_BONES])
		format |= Mesh::ARRAY_FORMAT_BONES;
	if (arrays[Mesh::ARRAY_WEIGHTS])
		format |= Mesh::ARRAY_FORMAT_WEIGHTS;
	if (arrays[Mesh::ARRAY_INDEX])
		format |= Mesh::ARRAY_FORMAT_INDEX;

	//check for custom stuff, probably could extract from flag, but this should work
	if ((format & Mesh::ARRAY_FORMAT_BONES) && (format & Mesh::ARRAY_FORMAT_WEIGHTS)) {
		const PackedVector3Array &vertex_array = arrays[Mesh::ARRAY_VERTEX];
		const PackedFloat32Array &weights_array = arrays[Mesh::ARRAY_WEIGHTS];
		if (vertex_array.size() * 8 == weights_array.size())
			format |= Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	}

	return format;
}