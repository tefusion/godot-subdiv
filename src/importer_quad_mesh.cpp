#include "importer_quad_mesh.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

ImporterQuadMesh::ImporterQuadMesh() {
}

ImporterQuadMesh::~ImporterQuadMesh() {
}

void ImporterQuadMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_surface"), &ImporterQuadMesh::add_surface);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &ImporterQuadMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &ImporterQuadMesh::_get_data);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "_set_data", "_get_data");
	//ClassDB::bind_method(D_METHOD("generate_trimesh"), &ImporterQuadMesh::generate_trimesh, DEFVAL(Ref<ArrayMesh>()));
}

void ImporterQuadMesh::add_surface(const Array &p_arrays, const Array &p_blend_shapes, const Ref<Material> &p_material, const String &p_name) {
	// ERR_FAIL_COND(p_arrays.size() != Mesh::ARRAY_MAX);
	Surface s;
	s.arrays = p_arrays;
	s.name = p_name;
	s.material = p_material;
	PackedVector3Array vertex_array = p_arrays[Mesh::ARRAY_VERTEX];
	int vertex_count = vertex_array.size();
	ERR_FAIL_COND(vertex_count == 0);

	// for (int i = 0; i < p_blend_shapes.size(); i++) {
	// 	Array bsdata = p_blend_shapes[i];
	// 	ERR_FAIL_COND(bsdata.size() != Mesh::ARRAY_MAX);
	// 	PackedVector3Array vertex_data = bsdata[Mesh::ARRAY_VERTEX];
	// 	ERR_FAIL_COND(vertex_data.size() != vertex_count);
	// 	Surface::BlendShape bs;
	// 	bs.arrays = bsdata;
	// 	s.blend_shape_data.push_back(bs);
	// }
	surfaces.push_back(s);
}

//generates Mesh arrays for add_surface_call
Array ImporterQuadMesh::generate_trimesh_arrays(int surface_index) {
	const Array &quad_arrays = surface_get_arrays(surface_index);
	ERR_FAIL_COND_V_MSG(quad_arrays.size() != ImporterQuadMesh::ARRAY_MAX, Array(), "Surface arrays of quad mesh corrupted, try reimporting.");

	const PackedVector3Array &quad_vertex_array = quad_arrays[ImporterQuadMesh::ARRAY_VERTEX];
	const PackedInt32Array &quad_index_array = quad_arrays[ImporterQuadMesh::ARRAY_INDEX];
	const PackedVector3Array &quad_normal_array = quad_arrays[ImporterQuadMesh::ARRAY_NORMAL];
	const PackedInt32Array &quad_uv_index_array = quad_arrays[ImporterQuadMesh::ARRAY_UV_INDEX];
	const PackedVector2Array &quad_uv_array = quad_arrays[ImporterQuadMesh::ARRAY_TEX_UV];
	//generate tri index array and new vertex array
	PackedVector3Array tri_vertex_array;
	PackedVector3Array tri_normal_array;
	PackedVector2Array tri_uv_array;
	PackedInt32Array tri_index_array;
	Array tri_arrays;
	tri_arrays.resize(Mesh::ARRAY_MAX);
	for (int quad_index = 0; quad_index < quad_index_array.size(); quad_index += 4) {
		//add vertices part of quad
		//after for loop unshared0 vertex will be at the positon quad_index in the new vertex_array in the SurfaceTool
		for (int single_quad_index = quad_index; single_quad_index < quad_index + 4; single_quad_index++) {
			tri_uv_array.append(quad_uv_array[quad_uv_index_array[single_quad_index]]);
			tri_normal_array.append(quad_normal_array[quad_index_array[single_quad_index]]);
			tri_vertex_array.append(quad_vertex_array[quad_index_array[single_quad_index]]);
		} //unshared0, shared0, unshared1, shared1

		//add triangle 1 with unshared0
		tri_index_array.append(quad_index);
		tri_index_array.append(quad_index + 1);
		tri_index_array.append(quad_index + 3);

		//add triangle 2 with unshared1
		tri_index_array.append(quad_index + 1);
		tri_index_array.append(quad_index + 2);
		tri_index_array.append(quad_index + 3);
	}

	tri_arrays[Mesh::ARRAY_VERTEX] = tri_vertex_array;
	tri_arrays[Mesh::ARRAY_INDEX] = tri_index_array;
	tri_arrays[Mesh::ARRAY_NORMAL] = tri_normal_array;
	tri_arrays[Mesh::ARRAY_TEX_UV] = tri_uv_array;
	return tri_arrays;
}

Array ImporterQuadMesh::surface_get_arrays(int p_index) const {
	ERR_FAIL_COND_V_MSG(p_index >= surfaces.size(), Array(), "Surface index above surface count.");
	return surfaces[p_index].arrays;
}

void ImporterQuadMesh::_set_data(const Dictionary &p_data) {
	clear();
	// if (p_data.has("blend_shape_names")) {
	// 	blend_shapes = p_data["blend_shape_names"];
	// }
	if (p_data.has("surfaces")) {
		Array surface_arr = p_data["surfaces"];
		for (int i = 0; i < surface_arr.size(); i++) {
			Dictionary s = surface_arr[i];
			ERR_CONTINUE(!s.has("arrays"));
			Array arr = s["arrays"];
			String name;
			if (s.has("name")) {
				name = s["name"];
			}
			// Array b_shapes;
			// if (s.has("b_shapes")) {
			// 	b_shapes = s["b_shapes"];
			// }
			Ref<Material> material;
			if (s.has("material")) {
				material = s["material"];
			}

			add_surface(arr, Array(), material, name);
		}
	}
}
Dictionary ImporterQuadMesh::_get_data() const {
	Dictionary data;
	// if (blend_shapes.size()) {
	// 	data["blend_shape_names"] = blend_shapes;
	// }
	Array surface_arr;
	for (int i = 0; i < surfaces.size(); i++) {
		Dictionary d;
		d["arrays"] = surfaces[i].arrays;
		// if (surfaces[i].blend_shape_data.size()) {
		// 	Array bs_data;
		// 	for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
		// 		bs_data.push_back(surfaces[i].blend_shape_data[j].arrays);
		// 	}
		// 	d["blend_shapes"] = bs_data;
		// }
		// if (surfaces[i].lods.size()) {
		// 	Dictionary lods;
		// 	for (int j = 0; j < surfaces[i].lods.size(); j++) {
		// 		lods[surfaces[i].lods[j].distance] = surfaces[i].lods[j].indices;
		// 	}
		// 	d["lods"] = lods;
		// }

		if (surfaces[i].material.is_valid()) {
			d["material"] = surfaces[i].material;
		}

		if (!surfaces[i].name.is_empty()) {
			d["name"] = surfaces[i].name;
		}
		surface_arr.push_back(d);
	}
	data["surfaces"] = surface_arr;
	return data;
}

void ImporterQuadMesh::clear() {
	surfaces.clear();
	//blend_shapes.clear();
}

int32_t ImporterQuadMesh::get_surface_count() {
	return surfaces.size();
}

String ImporterQuadMesh::surface_get_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
void ImporterQuadMesh::surface_set_name(int p_surface, const String &p_name) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
}

Ref<Material> ImporterQuadMesh::surface_get_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Ref<Material>());
	return surfaces[p_surface].material;
}

void ImporterQuadMesh::surface_set_material(int p_surface, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].material = p_material;
}

//return vertex array length of surface
int ImporterQuadMesh::surface_get_length(int p_surface) {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), -1);
	const PackedVector3Array &vertex_array = surfaces[p_surface].arrays[ImporterQuadMesh::ARRAY_VERTEX];
	return vertex_array.size();
}