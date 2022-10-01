#include "doctest.h"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/triangle_subdivider.hpp"
#include "test_utility_methods.hpp"

//I don't think a lot of unit tests are needed for this. If it outputs non empty data subdivision very likely worked since opensubdiv let the data through
TEST_CASE("subdivide once") {
	Array arr;
	PackedVector3Array vertex_array;
	vertex_array.push_back(Vector3(0, 0, 0));
	vertex_array.push_back(Vector3(0, 1, 0));
	vertex_array.push_back(Vector3(1, 1, 0));

	int32_t index_arr[] = { 0, 1, 2 };
	PackedInt32Array index_array = create_packed_int32_array(index_arr, 3);

	arr.resize(Mesh::ARRAY_MAX);
	arr[TopologyDataMesh::ARRAY_VERTEX] = vertex_array;
	arr[TopologyDataMesh::ARRAY_INDEX] = index_array;

	int32_t p_format = Mesh::ARRAY_FORMAT_VERTEX;
	p_format &= Mesh::ARRAY_FORMAT_INDEX;
	TriangleSubdivider subdivider;
	Array result = subdivider.get_subdivided_arrays(arr, 1, p_format, false);
	CHECK(result.size() == Mesh::ARRAY_MAX);
	const PackedVector3Array &result_vertex_array = result[Mesh::ARRAY_VERTEX];
	const PackedInt32Array &result_index_array = result[Mesh::ARRAY_INDEX];
	CHECK(result_vertex_array.size() != 0);
	CHECK(result_index_array.size() % 3 == 0);
}