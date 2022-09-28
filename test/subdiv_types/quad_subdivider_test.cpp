#include "doctest.h"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/subdiv_data_mesh.hpp"
#include "subdiv_types/quad_subdivider.hpp"

bool contains_null(PackedVector3Array arr) {
	for (int i = 0; i < arr.size(); i++) {
		if (&arr[i] == nullptr) {
			return true;
		}
	}
	return false;
}

bool contains_default(PackedVector3Array arr) {
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i] == Vector3(0, 0, 0)) {
			return true;
		}
	}
	return false;
}

bool equal_approx(PackedVector3Array result, PackedVector3Array expected) {
	if (result.size() != expected.size()) {
		return false;
	}

	for (int i = 0; i < result.size(); i++) {
		if (!result[i].is_equal_approx(expected[i])) {
			return false;
		}
	}
	return true;
}

//just checks that quad_subdivider outputs usable data
TEST_CASE("simple cube test") {
	Ref<SubdivDataMesh> a = ResourceLoader::get_singleton()->load("res://test/cube.tres");
	const Array arr = a->surface_get_data_arrays(0);
	int32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format &= Mesh::ARRAY_FORMAT_INDEX;
	QuadSubdivider quad_subdivider;
	Array result = quad_subdivider.get_subdivided_arrays(arr, 1, a->_surface_get_format(0), true);
	const PackedVector3Array &vertex_array = result[Mesh::ARRAY_VERTEX];
	const PackedVector3Array &normal_array = result[Mesh::ARRAY_NORMAL];
	const PackedInt32Array &index_array = result[Mesh::ARRAY_INDEX];
	CHECK(vertex_array.size() != 0);
	CHECK(index_array.size() % 3 == 0);
	CHECK(normal_array.size() == vertex_array.size());
	CHECK(!contains_null(normal_array));
	CHECK(!contains_default(normal_array)); //normal of 0,0,0 shouldn't happen
}

//TODO: after implementing subdivision baker compare with that here
TEST_CASE("compare with subdivided") {
	Ref<SubdivDataMesh> a = ResourceLoader::get_singleton()->load("res://test/cube.tres");
	const Array arr = a->surface_get_data_arrays(0);
	int32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format &= Mesh::ARRAY_FORMAT_INDEX;
	QuadSubdivider quad_subdivider;
	Array result = quad_subdivider.get_subdivided_topology_arrays(arr, 1, a->_surface_get_format(0), true);
	const PackedVector3Array &vertex_array = result[SubdivDataMesh::ARRAY_VERTEX];
	const PackedVector3Array &normal_array = result[SubdivDataMesh::ARRAY_NORMAL];
	const PackedInt32Array &index_array = result[SubdivDataMesh::ARRAY_INDEX];
	CHECK(vertex_array.size() != 0);
	CHECK(index_array.size() % 3 == 0);
	CHECK(normal_array.size() == vertex_array.size());
	Ref<SubdivDataMesh> expected = ResourceLoader::get_singleton()->load("res://test/cube_subdiv_1.tres");
	Array expected_arr = expected->surface_get_data_arrays(0);
	const PackedVector3Array &expected_vertex_array = expected_arr[SubdivDataMesh::ARRAY_VERTEX];
	const PackedInt32Array &expected_index_array = expected_arr[SubdivDataMesh::ARRAY_INDEX];

	CHECK(expected_index_array.size() == index_array.size());
}