#include "test_utility_methods.hpp"

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

PackedInt32Array create_packed_int32_array(int32_t arr[], int size) {
	PackedInt32Array packed_array;
	for (int i = 0; i < size; i++) {
		packed_array.push_back(arr[i]);
	}
	return packed_array;
}