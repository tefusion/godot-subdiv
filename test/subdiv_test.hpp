#ifndef SUBDIV_TEST_H
#define SUBDIV_TEST_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"

using namespace godot;
class SubdivTest : public RefCounted {
	GDCLASS(SubdivTest, RefCounted);

protected:
	static void _bind_methods();

public:
	int run_tests();
};

#endif
