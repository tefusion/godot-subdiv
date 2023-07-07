#include "doctest.h"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/quad_subdivider.hpp"
#include "test_utility_methods.hpp"

//just checks that quad_subdivider outputs usable data
TEST_CASE("simple cube test") {
	Ref<TopologyDataMesh> a = ResourceLoader::get_singleton()->load("res://test/cube.tres");
	const Array arr = a->surface_get_arrays(0);
	int32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format &= Mesh::ARRAY_FORMAT_INDEX;
	Ref<QuadSubdivider> quad_subdivider;
	quad_subdivider.instantiate();
	Array result = quad_subdivider->get_subdivided_arrays(arr, 1, a->surface_get_format(0), true);
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
	Ref<TopologyDataMesh> a = ResourceLoader::get_singleton()->load("res://test/cube.tres");
	const Array arr = a->surface_get_arrays(0);
	int32_t surface_format = Mesh::ARRAY_FORMAT_VERTEX;
	surface_format &= Mesh::ARRAY_FORMAT_INDEX;
	Ref<QuadSubdivider> quad_subdivider;
	quad_subdivider.instantiate();
	Array result = quad_subdivider->get_subdivided_topology_arrays(arr, 1, a->surface_get_format(0), true);
	const PackedVector3Array &vertex_array = result[TopologyDataMesh::ARRAY_VERTEX];
	const PackedVector3Array &normal_array = result[TopologyDataMesh::ARRAY_NORMAL];
	const PackedInt32Array &index_array = result[TopologyDataMesh::ARRAY_INDEX];
	CHECK(vertex_array.size() != 0);
	CHECK(index_array.size() % 3 == 0);
	CHECK(normal_array.size() == vertex_array.size());
	Ref<TopologyDataMesh> expected = ResourceLoader::get_singleton()->load("res://test/cube_subdiv_1.tres");
	Array expected_arr = expected->surface_get_arrays(0);
	const PackedVector3Array &expected_vertex_array = expected_arr[TopologyDataMesh::ARRAY_VERTEX];
	const PackedInt32Array &expected_index_array = expected_arr[TopologyDataMesh::ARRAY_INDEX];

	CHECK(expected_index_array.size() == index_array.size());
}

TEST_CASE("subdiv level zero") {
	Array arr;
	PackedVector3Array vertex_array;
	vertex_array.push_back(Vector3(0, 0, 0));
	vertex_array.push_back(Vector3(0, 1, 0));
	vertex_array.push_back(Vector3(1, 1, 0));
	vertex_array.push_back(Vector3(1, 0, 0));

	int32_t index_arr[] = { 0, 1, 2, 3 };
	PackedInt32Array index_array = create_packed_int32_array(index_arr, 4);

	arr.resize(Mesh::ARRAY_MAX);
	arr[TopologyDataMesh::ARRAY_VERTEX] = vertex_array;
	arr[TopologyDataMesh::ARRAY_INDEX] = index_array;

	int32_t p_format = Mesh::ARRAY_FORMAT_VERTEX;
	p_format &= Mesh::ARRAY_FORMAT_INDEX;
	Ref<QuadSubdivider> subdivider;
	subdivider.instantiate();
	Array result = subdivider->get_subdivided_arrays(arr, 0, p_format, false);
	CHECK(result.size() == Mesh::ARRAY_MAX); //TODO: make a test suite that calls all these for each input
	const PackedVector3Array &result_vertex_array = result[Mesh::ARRAY_VERTEX];
	const PackedInt32Array &result_index_array = result[Mesh::ARRAY_INDEX];
	CHECK(result_vertex_array.size() != 0);
	CHECK(result_vertex_array == vertex_array);
	CHECK(result_index_array.size() % 3 == 0);

	int32_t expected_index_arr[] = { 0, 1, 3, 1, 2, 3 };
	PackedInt32Array expected_index_array = create_packed_int32_array(expected_index_arr, 6);
	CHECK_EQ(expected_index_array, result_index_array);
}