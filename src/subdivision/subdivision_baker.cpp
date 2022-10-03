#include "subdivision_baker.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "quad_subdivider.hpp"
#include "triangle_subdivider.hpp"

Array SubdivisionBaker::get_baked_arrays(const Array &topology_arrays, int p_level, int64_t p_format, TopologyDataMesh::TopologyType topology_type) {
	switch (topology_type) {
		case TopologyDataMesh::QUAD: {
			QuadSubdivider subdivider;
			return subdivider.get_subdivided_arrays(topology_arrays, p_level, p_format, true);
		}

		case TopologyDataMesh::TRIANGLE: {
			TriangleSubdivider subdivider;
			return subdivider.get_subdivided_arrays(topology_arrays, p_level, p_format, true);
		}

		default:
			return Array();
	}
}

TypedArray<Array> SubdivisionBaker::get_baked_blend_shape_arrays(const Array &base_arrays, const Array &relative_topology_blend_shape_arrays,
		int32_t p_level, int64_t p_format, TopologyDataMesh::TopologyType topology_type) {
	Array blend_shape_arrays = base_arrays.duplicate(false);
	p_format &= ~Mesh::ARRAY_FORMAT_BONES;
	p_format &= ~Mesh::ARRAY_FORMAT_WEIGHTS;
	TypedArray<Array> baked_blend_shape_arrays;
	const PackedVector3Array &topology_vertex_array = base_arrays[TopologyDataMesh::ARRAY_VERTEX];
	for (int blend_shape_idx = 0; blend_shape_idx < relative_topology_blend_shape_arrays.size(); blend_shape_idx++) {
		const Array &single_blend_shape_arrays = relative_topology_blend_shape_arrays[blend_shape_idx];
		const PackedVector3Array &blend_shape_relative_vertex_array = single_blend_shape_arrays[0]; //expects relative data
		PackedVector3Array blend_shape_vertex_array_absolute = topology_vertex_array;
		for (int vertex_idx = 0; vertex_idx < topology_vertex_array.size(); vertex_idx++) {
			blend_shape_vertex_array_absolute[vertex_idx] += blend_shape_relative_vertex_array[vertex_idx];
		}

		blend_shape_arrays[Mesh::ARRAY_VERTEX] = blend_shape_vertex_array_absolute;

		Array full_baked_array = get_baked_arrays(blend_shape_arrays, p_level, p_format, topology_type);

		//Vertex, normal, tangent
		Array single_baked_blend_shape_array;
		single_baked_blend_shape_array.resize(Mesh::ARRAY_MAX);
		single_baked_blend_shape_array[Mesh::ARRAY_VERTEX] = full_baked_array[Mesh::ARRAY_VERTEX];
		single_baked_blend_shape_array[Mesh::ARRAY_NORMAL] = full_baked_array[Mesh::ARRAY_NORMAL];
		single_baked_blend_shape_array[Mesh::ARRAY_TANGENT] = full_baked_array[Mesh::ARRAY_TANGENT];
		baked_blend_shape_arrays.push_back(single_baked_blend_shape_array);
	}

	return baked_blend_shape_arrays;
}

Ref<ImporterMesh> SubdivisionBaker::get_importer_mesh(const Ref<ImporterMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level, bool bake_blendshapes) {
	Ref<ImporterMesh> mesh;
	if (!p_base.is_null()) {
		mesh = p_base;
	}
	if (mesh.is_null()) {
		mesh.instantiate();
	}

	if (bake_blendshapes) {
		for (int blend_shape_idx = 0; blend_shape_idx < p_topology_data_mesh->get_blend_shape_count(); blend_shape_idx++) {
			mesh->add_blend_shape(p_topology_data_mesh->get_blend_shape_name(blend_shape_idx));
		}
	}

	for (int surface_index = 0; surface_index < p_topology_data_mesh->get_surface_count(); surface_index++) {
		const Array &source_arrays = p_topology_data_mesh->surface_get_arrays(surface_index);
		int64_t p_format = p_topology_data_mesh->surface_get_format(surface_index);
		TopologyDataMesh::TopologyType topology_type = p_topology_data_mesh->surface_get_topology_type(surface_index);
		const String &surface_name = p_topology_data_mesh->surface_get_name(surface_index);
		const Ref<Material> &surface_material = p_topology_data_mesh->surface_get_material(surface_index);

		Array surface_baked_arrays = get_baked_arrays(source_arrays, p_level, p_format, topology_type);

		TypedArray<Array> baked_blend_shape_arrays;
		if (bake_blendshapes && p_topology_data_mesh->get_blend_shape_count() > 0) {
			baked_blend_shape_arrays = get_baked_blend_shape_arrays(source_arrays, p_topology_data_mesh->surface_get_blend_shape_arrays(surface_index),
					p_level, p_format, topology_type);
		}

		mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES, surface_baked_arrays, baked_blend_shape_arrays, Dictionary(), surface_material, surface_name, 0);
	}

	return mesh;
}

Ref<ArrayMesh> SubdivisionBaker::get_array_mesh(const Ref<ArrayMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level, bool generate_lods, bool bake_blendshapes) {
	Ref<ArrayMesh> mesh;
	Ref<ImporterMesh> importer_mesh;
	if (!p_base.is_null()) {
		mesh = p_base;
	}
	if (mesh.is_null()) {
		mesh.instantiate();
	}

	importer_mesh = get_importer_mesh(importer_mesh, p_topology_data_mesh, p_level, bake_blendshapes);
	if (generate_lods) {
		importer_mesh->generate_lods(UtilityFunctions::deg_to_rad(25), UtilityFunctions::deg_to_rad(60), Array());
	}
	mesh = importer_mesh->get_mesh(mesh);
	return mesh;
}

SubdivisionBaker::SubdivisionBaker() {}
SubdivisionBaker::~SubdivisionBaker() {}

void SubdivisionBaker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_baked_arrays", "topology_arrays", "subdivision_level", "format", "topology_type"), &SubdivisionBaker::get_baked_arrays);
	ClassDB::bind_method(D_METHOD("get_importer_mesh", "base", "topology_data_mesh", "subdivision_level"), &SubdivisionBaker::get_importer_mesh);
	ClassDB::bind_method(D_METHOD("get_array_mesh", "base", "topology_data_mesh", "subdivision_level", "generate_lods"), &SubdivisionBaker::get_array_mesh);
}