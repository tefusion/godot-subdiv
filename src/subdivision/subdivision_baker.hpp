#ifndef SUBDIVISION_BAKER_H
#define SUBDIVISION_BAKER_H

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/importer_mesh.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "resources/topology_data_mesh.hpp"

using namespace godot;
class SubdivisionBaker : public RefCounted {
	GDCLASS(SubdivisionBaker, RefCounted);

protected:
	static void _bind_methods();

public:
	Ref<ArrayMesh> get_array_mesh(const Ref<ArrayMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level, bool generate_lods);
	Ref<ImporterMesh> get_importer_mesh(const Ref<ImporterMesh> &p_base, const Ref<TopologyDataMesh> &p_topology_data_mesh, int32_t p_level);
	Array get_baked_arrays(const Array &topology_arrays, int32_t p_level, int64_t p_format, TopologyDataMesh::TopologyType topology_type);

	SubdivisionBaker();
	~SubdivisionBaker();
};

#endif
