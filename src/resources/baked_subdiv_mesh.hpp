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

class SubdivisionMesh;
class TopologyDataMesh;

//Uses a TopologyDataMesh in the background to function like a Mesh
class BakedSubdivMesh : public Mesh {
	GDCLASS(BakedSubdivMesh, Mesh);

	virtual int64_t _get_surface_count() const override;
	virtual int64_t _surface_get_array_len(int64_t index) const override;
	virtual int64_t _surface_get_array_index_len(int64_t index) const override;
	virtual Array _surface_get_arrays(int64_t index) const;
	virtual TypedArray<Array> _surface_get_blend_shape_arrays(int64_t index) const override;
	virtual Dictionary _surface_get_lods(int64_t index) const override;
	virtual int64_t _surface_get_format(int64_t index) const override;
	virtual int64_t _surface_get_primitive_type(int64_t index) const override;
	virtual void _surface_set_material(int64_t index, const Ref<Material> &material) override;
	virtual Ref<Material> _surface_get_material(int64_t index) const override;
	virtual int64_t _get_blend_shape_count() const override;
	virtual StringName _get_blend_shape_name(int64_t index) const override;
	virtual void _set_blend_shape_name(int64_t index, const StringName &name) override;
	virtual void _add_blend_shape_name(const StringName &p_name);
	virtual AABB _get_aabb() const override;

protected:
	Ref<TopologyDataMesh> data_mesh;
	SubdivisionMesh *subdiv_mesh;
	RID subdiv_mesh_rid;
	int subdiv_level = 0;

	void _update_subdiv();

	static void _bind_methods();

public:
	void set_data_mesh(Ref<TopologyDataMesh> p_data_mesh);
	Ref<TopologyDataMesh> get_data_mesh() const;
	void set_subdiv_level(int p_level);
	int get_subdiv_level() const;
	RID get_rid() const;
	BakedSubdivMesh();
	~BakedSubdivMesh();
};

#endif