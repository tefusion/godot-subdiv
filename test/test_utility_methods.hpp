#ifndef TEST_UTILITY_METHODS_H
#define TEST_UTILITY_METHODS_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"
using namespace godot;

bool contains_null(PackedVector3Array arr);
bool contains_default(PackedVector3Array arr);
bool equal_approx(PackedVector3Array result, PackedVector3Array expected);
PackedInt32Array create_packed_int32_array(int32_t arr[], int size);

#endif