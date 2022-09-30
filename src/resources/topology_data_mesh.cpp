#include "topology_data_mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "subdivision/subdivision_mesh.hpp"
#include "subdivision/subdivision_server.hpp"

void TopologyDataMesh::add_surface(const Array &p_arrays, const Array &p_blend_shapes, const Ref<Material> &p_material, const String &p_name, int32_t p_format) {
	ERR_FAIL_COND(p_arrays.size() != TopologyDataMesh::ARRAY_MAX);
	Surface s;
	s.arrays = p_arrays;
	s.name = p_name;
	s.material = p_material;
	s.format = p_format;
	PackedVector3Array vertex_array = p_arrays[TopologyDataMesh::ARRAY_VERTEX];
	int vertex_count = vertex_array.size();
	ERR_FAIL_COND(vertex_count == 0);

	for (int i = 0; i < p_blend_shapes.size(); i++) {
		Array bsdata = p_blend_shapes[i];
		ERR_FAIL_COND(bsdata.size() != TopologyDataMesh::ARRAY_MAX);
		PackedVector3Array vertex_data = bsdata[TopologyDataMesh::ARRAY_VERTEX];
		ERR_FAIL_COND(vertex_data.size() != vertex_count);
		s.blend_shape_data.push_back(bsdata);
	}

	surfaces.push_back(s);
}

//this method gives the actual stored vertices
Array TopologyDataMesh::surface_get_arrays(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), Array());
	return surfaces[p_index].arrays;
}

void TopologyDataMesh::_set_data(const Dictionary &p_data) {
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
			int32_t format = s["format"];
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

			add_surface(arr, b_shapes, material, name, format);
		}
	}
}
Dictionary TopologyDataMesh::_get_data() const {
	Dictionary data;
	if (blend_shapes.size()) {
		data["blend_shape_names"] = blend_shapes;
	}
	Array surface_arr;
	for (int i = 0; i < surfaces.size(); i++) {
		Dictionary d;
		d["arrays"] = surfaces[i].arrays;
		d["format"] = surfaces[i].format;
		if (surfaces[i].blend_shape_data.size()) {
			Array bs_data;
			for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
				bs_data.push_back(surfaces[i].blend_shape_data[j]);
			}
			d["blend_shapes"] = bs_data;
		}
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

void TopologyDataMesh::clear() {
	surfaces.clear();
	blend_shapes.clear();
}

int64_t TopologyDataMesh::get_surface_count() const {
	return surfaces.size();
}

int64_t TopologyDataMesh::surface_get_format(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), 0);
	return surfaces[index].format;
}

void TopologyDataMesh::surface_set_material(int64_t index, const Ref<Material> &material) {
	ERR_FAIL_INDEX(index, surfaces.size());
	surfaces.write[index].material = material;
}
Ref<Material> TopologyDataMesh::surface_get_material(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), Ref<Material>());
	Ref<Material> a = surfaces[index].material;
	return surfaces[index].material;
}

int64_t TopologyDataMesh::get_blend_shape_count() const {
	return blend_shapes.size();
}

Array TopologyDataMesh::surface_get_blend_shape_arrays(int64_t surface_index) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Array());
	return surfaces[surface_index].blend_shape_data;
}

Array TopologyDataMesh::surface_get_single_blend_shape_array(int64_t surface_index, int64_t blend_shape_idx) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Array());
	ERR_FAIL_INDEX_V(blend_shape_idx, surfaces[surface_index].blend_shape_data.size(), Array());
	return surfaces[surface_index].blend_shape_data[blend_shape_idx];
}

StringName TopologyDataMesh::get_blend_shape_name(int64_t index) const {
	ERR_FAIL_INDEX_V(index, blend_shapes.size(), StringName());
	return blend_shapes[index];
}
void TopologyDataMesh::set_blend_shape_name(int64_t index, const StringName &p_name) {
	ERR_FAIL_INDEX(index, blend_shapes.size());
	ERR_FAIL_COND(blend_shapes.has(p_name)); //godot has a function here to generate a name for this, this should be enough for now
	blend_shapes[index] = p_name;
}

//only use after adding blend shape to surface
void TopologyDataMesh::add_blend_shape_name(const StringName &p_name) {
	blend_shapes.push_back(p_name);
}

String TopologyDataMesh::surface_get_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
void TopologyDataMesh::surface_set_name(int p_surface, const String &p_name) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
}

//return vertex array length of surface
int TopologyDataMesh::surface_get_length(int p_surface) {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), -1);
	const PackedVector3Array &vertex_array = surfaces[p_surface].arrays[TopologyDataMesh::ARRAY_VERTEX];
	return vertex_array.size();
}

TopologyDataMesh::TopologyDataMesh() {
}

TopologyDataMesh::~TopologyDataMesh() {
}

void TopologyDataMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_surface"), &TopologyDataMesh::add_surface);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &TopologyDataMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &TopologyDataMesh::_get_data);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "_set_data", "_get_data");

	//virtuals of mesh
	TopologyDataMesh::register_virtuals<Mesh>();
}