#include "subdiv_data_mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "subdiv_types/subdivision_mesh.hpp"
#include "subdivision_server.hpp"

void SubdivDataMesh::add_surface(const Array &p_arrays, const Array &p_blend_shapes, const Ref<Material> &p_material, const String &p_name) {
	ERR_FAIL_COND(p_arrays.size() != SubdivDataMesh::ARRAY_MAX);
	Surface s;
	s.arrays = p_arrays;
	s.name = p_name;
	s.material = p_material;
	PackedVector3Array vertex_array = p_arrays[SubdivDataMesh::ARRAY_VERTEX];
	int vertex_count = vertex_array.size();
	ERR_FAIL_COND(vertex_count == 0);

	for (int i = 0; i < p_blend_shapes.size(); i++) {
		Array bsdata = p_blend_shapes[i];
		ERR_FAIL_COND(bsdata.size() != SubdivDataMesh::ARRAY_MAX);
		PackedVector3Array vertex_data = bsdata[SubdivDataMesh::ARRAY_VERTEX];
		ERR_FAIL_COND(vertex_data.size() != vertex_count);
		s.blend_shape_data.push_back(bsdata);
	}

	surfaces.push_back(s);
}

//generates Mesh arrays for add_surface_call
Array SubdivDataMesh::generate_trimesh_arrays(int64_t surface_index) const {
	const Array &quad_arrays = surface_get_data_arrays(surface_index);
	return generate_trimesh_arrays_from_quad_arrays(quad_arrays);
}

Array SubdivDataMesh::generate_trimesh_arrays_from_quad_arrays(const Array &quad_arrays) const {
	ERR_FAIL_COND_V_MSG(quad_arrays.size() != SubdivDataMesh::ARRAY_MAX, Array(), "Surface arrays of quad mesh corrupted, try reimporting.");
	const PackedVector3Array &quad_vertex_array = quad_arrays[SubdivDataMesh::ARRAY_VERTEX];
	const PackedInt32Array &quad_index_array = quad_arrays[SubdivDataMesh::ARRAY_INDEX];
	const PackedVector3Array &quad_normal_array = quad_arrays[SubdivDataMesh::ARRAY_NORMAL];
	const PackedInt32Array &quad_uv_index_array = quad_arrays[SubdivDataMesh::ARRAY_UV_INDEX];
	const PackedVector2Array &quad_uv_array = quad_arrays[SubdivDataMesh::ARRAY_TEX_UV];
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
			if (quad_uv_array.size()) {
				tri_uv_array.append(quad_uv_array[quad_uv_index_array[single_quad_index]]);
			}

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

//this method gives the actual stored vertices
Array SubdivDataMesh::surface_get_data_arrays(int64_t p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), Array());
	return surfaces[p_index].arrays;
}

Array SubdivDataMesh::surface_get_blend_shape_data_arrays(int64_t surface_index) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Array());
	return surfaces[surface_index].blend_shape_data;
}

Array SubdivDataMesh::surface_get_single_blend_shape_data_array(int64_t surface_index, int64_t blend_shape_idx) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Array());
	ERR_FAIL_INDEX_V(blend_shape_idx, surfaces[surface_index].blend_shape_data.size(), Array());
	return surfaces[surface_index].blend_shape_data[blend_shape_idx];
}

int64_t SubdivDataMesh::get_blend_shape_data_count() const {
	return blend_shapes.size();
}

void SubdivDataMesh::_set_data(const Dictionary &p_data) {
	clear();
	if (p_data.has("blend_shape_names")) {
		blend_shapes = p_data["blend_shape_names"];
	}
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
			Array b_shapes;
			if (s.has("blend_shapes")) {
				b_shapes = s["blend_shapes"];
			}
			Ref<Material> material;
			if (s.has("material")) {
				material = s["material"];
			}

			add_surface(arr, b_shapes, material, name);
		}
	}
}
Dictionary SubdivDataMesh::_get_data() const {
	Dictionary data;
	if (blend_shapes.size()) {
		data["blend_shape_names"] = blend_shapes;
	}
	Array surface_arr;
	for (int i = 0; i < surfaces.size(); i++) {
		Dictionary d;
		d["arrays"] = surfaces[i].arrays;
		if (surfaces[i].blend_shape_data.size()) {
			Array bs_data;
			for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
				bs_data.push_back(surfaces[i].blend_shape_data[j]);
			}
			d["blend_shapes"] = bs_data;
		}
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

void SubdivDataMesh::clear() {
	surfaces.clear();
	blend_shapes.clear();
}

int64_t SubdivDataMesh::_get_surface_count() const {
	return surfaces.size();
}

//!!the following methods give fake or empty data to work as mesh!!
int64_t SubdivDataMesh::_surface_get_array_len(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), 0);
	PackedInt32Array index_array = surfaces[index].arrays[Mesh::ARRAY_INDEX];
	return index_array.size() / 4 * 6;
}
int64_t SubdivDataMesh::_surface_get_array_index_len(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), 0);
	PackedInt32Array index_array = surfaces[index].arrays[Mesh::ARRAY_INDEX];
	return index_array.size() / 4 * 6;
}

Array SubdivDataMesh::_surface_get_arrays(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), Array());
	return generate_trimesh_arrays(index);
}
Array SubdivDataMesh::_surface_get_blend_shape_arrays(int64_t surface_index) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Array());
	return surfaces[surface_index].blend_shape_data;
}
Dictionary SubdivDataMesh::_surface_get_lods(int64_t index) const {
	//TODO:
	return Dictionary();
}

int64_t SubdivDataMesh::_surface_get_format(int64_t index) const {
	//TODO:
	return 1;
}

int64_t SubdivDataMesh::_surface_get_primitive_type(int64_t index) const {
	return Mesh::PRIMITIVE_TRIANGLES;
}

void SubdivDataMesh::_surface_set_material(int64_t index, const Ref<Material> &material) {
	ERR_FAIL_INDEX(index, surfaces.size());
	surfaces.write[index].material = material;
}
Ref<Material> SubdivDataMesh::_surface_get_material(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), Ref<Material>());
	Ref<Material> a = surfaces[index].material;
	return surfaces[index].material;
}

int64_t SubdivDataMesh::_get_blend_shape_count() const {
	return blend_shapes.size();
}
StringName SubdivDataMesh::_get_blend_shape_name(int64_t index) const {
	ERR_FAIL_INDEX_V(index, blend_shapes.size(), StringName());
	return blend_shapes[index];
}
void SubdivDataMesh::_set_blend_shape_name(int64_t index, const StringName &p_name) {
	ERR_FAIL_INDEX(index, blend_shapes.size());
	ERR_FAIL_COND(blend_shapes.has(p_name)); //godot has a function here to generate a name for this, this should be enough for now
	blend_shapes[index] = p_name;
}

//only use after adding blend shape to surface
void SubdivDataMesh::_add_blend_shape_name(const StringName &p_name) {
	blend_shapes.push_back(p_name);
}

AABB SubdivDataMesh::_get_aabb() const {
	//TODO:
	return AABB();
}

String SubdivDataMesh::surface_get_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
void SubdivDataMesh::surface_set_name(int p_surface, const String &p_name) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
}

//return vertex array length of surface
int SubdivDataMesh::surface_get_length(int p_surface) {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), -1);
	const PackedVector3Array &vertex_array = surfaces[p_surface].arrays[SubdivDataMesh::ARRAY_VERTEX];
	return vertex_array.size();
}

SubdivDataMesh::SubdivDataMesh() {
}

SubdivDataMesh::~SubdivDataMesh() {
}

void SubdivDataMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_surface"), &SubdivDataMesh::add_surface);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &SubdivDataMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &SubdivDataMesh::_get_data);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "_set_data", "_get_data");
	//ClassDB::bind_method(D_METHOD("generate_trimesh"), &SubdivDataMesh::generate_trimesh, DEFVAL(Ref<ArrayMesh>()));

	//virtuals of mesh
	SubdivDataMesh::register_virtuals<Mesh>();
}