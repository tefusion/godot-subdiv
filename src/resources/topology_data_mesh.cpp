#include "topology_data_mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

void TopologyDataMesh::add_surface(const Array &p_arrays, const Dictionary &p_lods, const Array &p_blend_shapes, const Ref<Material> &p_material,
		const String &p_name, BitField<Mesh::ArrayFormat> p_format, TopologyType p_topology_type) {
	ERR_FAIL_COND(p_arrays.size() != TopologyDataMesh::ARRAY_MAX);
	Surface s;
	s.arrays = p_arrays;
	s.name = p_name;
	s.material = p_material;
	s.format = p_format;
	s.topology_type = p_topology_type;
	s.lods = p_lods;
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
	emit_changed();
}

Array TopologyDataMesh::surface_get_arrays(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Array());
	return surfaces[p_surface].arrays;
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
			int32_t topology_type_num = 0;
			if (s.has("topology_type")) {
				topology_type_num = s["topology_type"];
			}
			Dictionary lods;
			if (s.has("lods")) {
				lods = s["lods"];
			}

			TopologyType topology_type = static_cast<TopologyType>(topology_type_num);

			add_surface(arr, lods, b_shapes, material, name, format, topology_type);
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
		d["topology_type"] = surfaces[i].topology_type;
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

		if (!surfaces[i].lods.is_empty()) {
			d["lods"] = surfaces[i].lods;
		}

		surface_arr.push_back(d);
	}
	data["surfaces"] = surface_arr;
	return data;
}

void TopologyDataMesh::clear() {
	surfaces.clear();
	blend_shapes.clear();
	emit_changed();
}

int64_t TopologyDataMesh::get_surface_count() const {
	return surfaces.size();
}

BitField<Mesh::ArrayFormat> TopologyDataMesh::surface_get_format(int64_t index) const {
	ERR_FAIL_INDEX_V(index, get_surface_count(), 0);
	return surfaces[index].format;
}

void TopologyDataMesh::surface_set_material(int64_t index, const Ref<Material> &material) {
	ERR_FAIL_INDEX(index, surfaces.size());
	surfaces.write[index].material = material;
	emit_changed();
}
Ref<Material> TopologyDataMesh::surface_get_material(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), Ref<Material>());
	Ref<Material> a = surfaces[index].material;
	return surfaces[index].material;
}

void TopologyDataMesh::surface_set_topology_type(int64_t index, TopologyType p_topology_type) {
	ERR_FAIL_INDEX(index, surfaces.size());
	surfaces.write[index].topology_type = p_topology_type;
	emit_changed();
}

TopologyDataMesh::TopologyType TopologyDataMesh::surface_get_topology_type(int64_t index) const {
	ERR_FAIL_INDEX_V(index, surfaces.size(), TopologyType::QUAD);
	return surfaces[index].topology_type;
}

Dictionary TopologyDataMesh::surface_get_lods(int64_t surface_index) const {
	ERR_FAIL_INDEX_V(surface_index, surfaces.size(), Dictionary());

	return surfaces[surface_index].lods;
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
	StringName shape_name = p_name;
	if (blend_shapes.has(shape_name)) {
		//iterator code from godot, makes sure even if name exists blendshape added by generating different name
		int count = 2;
		do {
			shape_name = String(p_name) + " " + itos(count);
			count++;
		} while (blend_shapes.has(shape_name));
	}
	blend_shapes.push_back(shape_name);
}

void TopologyDataMesh::add_surface_from_arrays(TopologyType p_topology_type, const Array &p_arrays, const Array &p_blend_shapes, const Dictionary &p_lods, BitField<Mesh::ArrayFormat> p_format) {
	add_surface(p_arrays, p_lods, p_blend_shapes, Ref<Material>(), "", p_format, p_topology_type);
}

String TopologyDataMesh::surface_get_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
void TopologyDataMesh::surface_set_name(int p_surface, const String &p_name) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
}

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
	ClassDB::bind_method(D_METHOD("_set_data", "data"), &TopologyDataMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &TopologyDataMesh::_get_data);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "_set_data", "_get_data");
	ClassDB::bind_method(D_METHOD("add_surface", "p_arrays", "p_lods", "p_blend_shapes", "p_material", "p_name", "p_format", "p_topology_type"), &TopologyDataMesh::add_surface);
	ClassDB::bind_method(D_METHOD("add_surface_from_arrays", "topology_type", "arrays", "blend_shapes", "lods", "format"), &TopologyDataMesh::add_surface_from_arrays, DEFVAL(Array()), DEFVAL(Dictionary()), DEFVAL(Mesh::ARRAY_FORMAT_VERTEX));
	ClassDB::bind_method(D_METHOD("surface_get_arrays", "surface_index"), &TopologyDataMesh::surface_get_arrays);
	ClassDB::bind_method(D_METHOD("clear"), &TopologyDataMesh::clear);
	ClassDB::bind_method(D_METHOD("get_surface_count"), &TopologyDataMesh::get_surface_count);
	ClassDB::bind_method(D_METHOD("surface_get_format", "index"), &TopologyDataMesh::surface_get_format);
	ClassDB::bind_method(D_METHOD("surface_set_material", "index", "material"), &TopologyDataMesh::surface_set_material);
	ClassDB::bind_method(D_METHOD("surface_set_topology_type", "index", "p_topology_type"), &TopologyDataMesh::surface_set_topology_type);
	ClassDB::bind_method(D_METHOD("surface_get_topology_type", "index"), &TopologyDataMesh::surface_get_topology_type);
	ClassDB::bind_method(D_METHOD("surface_get_lods", "surface_index"), &TopologyDataMesh::surface_get_lods);
	ClassDB::bind_method(D_METHOD("get_blend_shape_count"), &TopologyDataMesh::get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("surface_get_blend_shape_arrays", "surface_index"), &TopologyDataMesh::surface_get_blend_shape_arrays);
	ClassDB::bind_method(D_METHOD("surface_get_single_blend_shape_array", "surface_index", "blend_shape_idx"), &TopologyDataMesh::surface_get_single_blend_shape_array);
	ClassDB::bind_method(D_METHOD("get_blend_shape_name", "index"), &TopologyDataMesh::get_blend_shape_name);
	ClassDB::bind_method(D_METHOD("set_blend_shape_name", "index", "p_name"), &TopologyDataMesh::set_blend_shape_name);
	ClassDB::bind_method(D_METHOD("add_blend_shape_name", "p_name"), &TopologyDataMesh::add_blend_shape_name);
	ClassDB::bind_method(D_METHOD("surface_get_name", "p_surface"), &TopologyDataMesh::surface_get_name);
	ClassDB::bind_method(D_METHOD("surface_set_name", "p_surface", "p_name"), &TopologyDataMesh::surface_set_name);
	ClassDB::bind_method(D_METHOD("surface_get_length", "p_surface"), &TopologyDataMesh::surface_get_length);

	//TopologyType
	BIND_ENUM_CONSTANT(QUAD);
	BIND_ENUM_CONSTANT(TRIANGLE);

	//ArrayType
	BIND_ENUM_CONSTANT(ARRAY_VERTEX);
	BIND_ENUM_CONSTANT(ARRAY_NORMAL);
	BIND_ENUM_CONSTANT(ARRAY_TANGENT);
	BIND_ENUM_CONSTANT(ARRAY_COLOR);
	BIND_ENUM_CONSTANT(ARRAY_TEX_UV);
	BIND_ENUM_CONSTANT(ARRAY_TEX_UV2);
	BIND_ENUM_CONSTANT(ARRAY_CUSTOM0);
	BIND_ENUM_CONSTANT(ARRAY_CUSTOM1);
	BIND_ENUM_CONSTANT(ARRAY_CUSTOM2);
	BIND_ENUM_CONSTANT(ARRAY_CUSTOM3);
	BIND_ENUM_CONSTANT(ARRAY_BONES);
	BIND_ENUM_CONSTANT(ARRAY_WEIGHTS);
	BIND_ENUM_CONSTANT(ARRAY_INDEX);
	BIND_ENUM_CONSTANT(ARRAY_UV_INDEX);
	BIND_ENUM_CONSTANT(ARRAY_MAX);
}