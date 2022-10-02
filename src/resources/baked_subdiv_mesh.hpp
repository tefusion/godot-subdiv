#ifndef BAKED_SUBDIV_MESH_H
#define BAKED_SUBDIV_MESH_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/templates/vector.hpp"

using namespace godot;

class TopologyDataMesh;

//Uses a TopologyDataMesh in the background to function like a Mesh
class BakedSubdivMesh : public ArrayMesh {
	GDCLASS(BakedSubdivMesh, ArrayMesh);

protected:
	Ref<TopologyDataMesh> data_mesh;
	int subdiv_level = 0;
	void _update_subdiv();
	void _clear();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret);

	static void _bind_methods();

public:
	void set_data_mesh(Ref<TopologyDataMesh> p_data_mesh);
	Ref<TopologyDataMesh> get_data_mesh() const;
	void set_subdiv_level(int p_level);
	int get_subdiv_level() const;
	BakedSubdivMesh();
	~BakedSubdivMesh();
};

#endif