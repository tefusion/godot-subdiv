/*
Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.
Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef SUBDIV_MESH_INSTANCE_3D_H
#define SUBDIV_MESH_INSTANCE_3D_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/binder_common.hpp"

#include "godot_cpp/classes/immediate_mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/skin.hpp"
#include "godot_cpp/classes/skin_reference.hpp"
#include "resources/topology_data_mesh.hpp"
#include "subdivision/subdivision_mesh.hpp"

using namespace godot;

class SubdivMeshInstance3D : public GeometryInstance3D {
	GDCLASS(SubdivMeshInstance3D, GeometryInstance3D)

protected:
	Ref<TopologyDataMesh> mesh;
	SubdivisionMesh *subdiv_mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path = NodePath("..");

	Vector<Ref<Material>> surface_materials;
	int32_t subdiv_level;

	Vector<Array> cached_data_array; //array of surfaces after blend shapes are applied, if empty (if no blendshapes) getter will return normal data array
	HashMap<StringName, int> blend_shape_properties;
	Vector<float> blend_shape_tracks;
	Vector<Ref<Material>> surface_override_materials;

	void _resolve_skeleton_path();
	void _update_subdiv();
	Array _get_cached_data_array(int p_surface) const;
	void _init_cached_data_array();
	void _mesh_changed(); //if actual mesh property changes
	void _subdiv_mesh_changed(); //if subdiv level changes, just needs to reapply override materials, cached_data stays

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &return_value) const;

public:
	void set_mesh(const Ref<TopologyDataMesh> &p_mesh);
	Ref<TopologyDataMesh> get_mesh() const;

	void set_skin(const Ref<Skin> &p_skin);
	Ref<Skin> get_skin() const;

	void set_surface_material(int p_idx, const Ref<Material> &p_material);
	Ref<Material> get_surface_material(int p_idx) const;

	void set_skeleton_path(const NodePath &p_path);
	NodePath get_skeleton_path() const;

	void set_subdiv_level(int p_level);
	int32_t get_subdiv_level();

	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	int get_surface_override_material_count() const;
	void set_surface_override_material(int p_surface, const Ref<Material> &p_material);
	Ref<Material> get_surface_override_material(int p_surface) const;

	SubdivMeshInstance3D();
	~SubdivMeshInstance3D();

protected:
	void _update_skinning();
	void _update_subdiv_mesh_vertices(int p_surface, const PackedVector3Array &vertex_array);
};
;

#endif