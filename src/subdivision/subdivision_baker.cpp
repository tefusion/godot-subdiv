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

Ref<ImporterMesh> SubdivisionBaker::get_importer_mesh(const Ref<ImporterMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level) {
	Ref<ImporterMesh> mesh;
	if (!p_base.is_null()) {
		mesh = p_base;
	}
	if (mesh.is_null()) {
		mesh.instantiate();
	}

	for (int surface_index = 0; surface_index < p_topology_data_mesh->get_surface_count(); surface_index++) {
		const Array &source_arrays = p_topology_data_mesh->surface_get_arrays(surface_index);
		int64_t p_format = p_topology_data_mesh->surface_get_format(surface_index);
		TopologyDataMesh::TopologyType topology_type = p_topology_data_mesh->surface_get_topology_type(surface_index);
		const String &surface_name = p_topology_data_mesh->surface_get_name(surface_index);
		const Ref<Material> &surface_material = p_topology_data_mesh->surface_get_material(surface_index);

		Array surface_baked_arrays = get_baked_arrays(source_arrays, p_level, p_format, topology_type);
		mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES, surface_baked_arrays, Array(), Dictionary(), surface_material, surface_name, 0);
	}

	return mesh;
}

//does not bake blendshapes
Ref<ArrayMesh> SubdivisionBaker::get_array_mesh(const Ref<ArrayMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level, bool generate_lods) {
	Ref<ArrayMesh> mesh;
	Ref<ImporterMesh> importer_mesh;
	if (!p_base.is_null()) {
		mesh = p_base;
	}
	if (mesh.is_null()) {
		mesh.instantiate();
	}

	importer_mesh = get_importer_mesh(importer_mesh, p_topology_data_mesh, p_level);
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