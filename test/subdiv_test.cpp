#define DOCTEST_CONFIG_IMPLEMENT
#include "subdiv_test.hpp"
#include "doctest.h"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

int SubdivTest::run_tests() {
	doctest::Context ctx;
	ctx.setOption("abort-after", 5);
	int res = ctx.run();
	if (ctx.shouldExit())
		return res;
	return res;
}

void SubdivTest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("run_tests"), &SubdivTest::run_tests);
}