#include "subdivider.hpp"

#include "godot_cpp/classes/mesh_data_tool.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/variant/builtin_types.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "resources/topology_data_mesh.hpp"

//debug
// #include <chrono>
// using namespace std::chrono;

using namespace OpenSubdiv;
typedef Far::TopologyDescriptor Descriptor;

Subdivider::TopologyData::TopologyData(const Array &p_mesh_arrays, int32_t p_format) {
	vertex_array = p_mesh_arrays[TopologyDataMesh::ARRAY_VERTEX];
	index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_INDEX];
	if (p_format & Mesh::ARRAY_FORMAT_TEX_UV) {
		uv_array = p_mesh_arrays[TopologyDataMesh::ARRAY_TEX_UV];
		uv_index_array = p_mesh_arrays[TopologyDataMesh::ARRAY_UV_INDEX];
	}
	if (p_format & Mesh::ARRAY_FORMAT_BONES && p_format & Mesh::ARRAY_FORMAT_WEIGHTS) {
		bones_array = p_mesh_arrays[TopologyDataMesh::ARRAY_BONES];
		weights_array = p_mesh_arrays[TopologyDataMesh::ARRAY_WEIGHTS];
	}

	index_count = index_array.size();
	face_count = index_array.size() / 4;
	vertex_count = vertex_array.size();
	uv_count = uv_index_array.size();
	bone_count = bones_array.size();
	weight_count = weights_array.size();
}

struct Vertex {
	void Clear() { x = y = z = 0; }
	void AddWithWeight(Vertex const &src, float weight) {
		x += weight * src.x;
		y += weight * src.y;
		z += weight * src.z;
	}
	float x, y, z;
};

struct VertexUV {
	void Clear() {
		u = v = 0.0f;
	}

	void AddWithWeight(VertexUV const &src, float weight) {
		u += weight * src.u;
		v += weight * src.v;
	}

	// Basic 'uv' layout channel
	float u, v;
};

struct VertexBones {
	void Clear() {
		b0 = b1 = b2 = b3 = 0;
	}

	void AddWithWeight(VertexBones const &src, float weight) {
		b0 = src.b0;
		b1 = src.b1;
		b2 = src.b2;
		b3 = src.b3;
	}

	int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
};
struct VertexWeights {
	void Clear() {
		w0 = w1 = w2 = w3 = 0.0f;
	}

	void AddWithWeight(VertexWeights const &src, float weight) {
		w0 += weight * src.w0;
		w1 += weight * src.w1;
		w2 += weight * src.w2;
		w3 += weight * src.w3;
	}

	float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.f;
};

Descriptor Subdivider::_create_topology_descriptor(const int num_channels, Vector<int> &subdiv_face_vertex_count, Descriptor::FVarChannel *channels) {
	Descriptor desc;
	desc.numVertices = topology_data.vertex_count;
	desc.numFaces = topology_data.face_count;
	desc.vertIndicesPerFace = topology_data.index_array.ptr();

	subdiv_face_vertex_count = _get_face_vertex_count();

	desc.numVertsPerFace = subdiv_face_vertex_count.ptr();

	if (num_channels > 0) {
		channels[Channels::UV].numValues = topology_data.uv_count;
		channels[Channels::UV].valueIndices = topology_data.uv_index_array.ptr();
		desc.numFVarChannels = num_channels;
		desc.fvarChannels = channels;
	}
	return desc;
}

Far::TopologyRefiner *Subdivider::_create_topology_refiner(const int32_t p_level, const int num_channels) {
	//create descriptor,
	Vector<int> subdiv_face_vertex_count;
	Descriptor::FVarChannel channels[num_channels];
	Descriptor desc = _create_topology_descriptor(num_channels, subdiv_face_vertex_count, channels);

	Sdc::SchemeType type = _get_refiner_type();
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

	Far::TopologyRefiner *refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
	//ERR_FAIL_COND(!refiner);
	ERR_FAIL_COND_V(!refiner, nullptr);

	Far::TopologyRefiner::UniformOptions refine_options(p_level);
	refine_options.fullTopologyInLastLevel = true;
	refiner->RefineUniform(refine_options);

	topology_data.vertex_count = refiner->GetNumVerticesTotal();
	if (num_channels) {
		topology_data.uv_count = refiner->GetNumFVarValuesTotal(Channels::UV);
	}

	return refiner;
}
void Subdivider::_create_subdivision_vertices(Far::TopologyRefiner *refiner, const int p_level, const bool face_varying_data) {
	topology_data.vertex_array.resize(topology_data.vertex_count);
	// Interpolate vertex primvar data
	Far::PrimvarRefiner primvar_refiner(*refiner);
	//vertices
	Vertex *src = (Vertex *)topology_data.vertex_array.ptr();
	for (int level = 0; level < p_level; ++level) {
		Vertex *dst = src + refiner->GetLevel(level).GetNumVertices();
		primvar_refiner.Interpolate(level + 1, src, dst);
		src = dst;
	}

	if (face_varying_data) { //could be handled in the same loop as vertices, but then not usable with just updating vertices
		topology_data.uv_array.resize(topology_data.uv_count);
		VertexUV *src_uv = (VertexUV *)topology_data.uv_array.ptr();
		//face varying data like uv's, weights etc.
		for (int level = 0; level < p_level; ++level) {
			VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(Channels::UV);
			primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, Channels::UV);
			src_uv = dst_uv;
		}
	}
}

Array Subdivider::get_subdivided_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	subdivide(p_arrays, p_level, p_format, calculate_normals);
	return _get_triangle_arrays();
}

Array Subdivider::get_subdivided_topology_arrays(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	ERR_FAIL_COND_V(p_level <= 0, Array());
	subdivide(p_arrays, p_level, p_format, calculate_normals);
	Array arr;
	arr.resize(TopologyDataMesh::ARRAY_MAX);
	arr[TopologyDataMesh::ARRAY_VERTEX] = topology_data.vertex_array;
	arr[TopologyDataMesh::ARRAY_NORMAL] = topology_data.normal_array;
	arr[TopologyDataMesh::ARRAY_TEX_UV] = topology_data.uv_array;
	arr[TopologyDataMesh::ARRAY_UV_INDEX] = topology_data.uv_index_array;
	arr[TopologyDataMesh::ARRAY_INDEX] = topology_data.index_array;
	arr[TopologyDataMesh::ARRAY_BONES] = topology_data.bones_array;
	arr[TopologyDataMesh::ARRAY_WEIGHTS] = topology_data.weights_array;
	return arr;
}

void Subdivider::subdivide(const Array &p_arrays, int p_level, int32_t p_format, bool calculate_normals) {
	ERR_FAIL_COND(p_level < 0);
	bool use_uv = p_format & Mesh::ARRAY_FORMAT_TEX_UV;

	topology_data = TopologyData(p_arrays, p_format);
	if (p_level == 0) {
		return;
	}

	const int num_channels = use_uv; //TODO: maybe add channel for tex uv 2
	const bool face_varying_data = num_channels > 0;

	Far::TopologyRefiner *refiner = _create_topology_refiner(p_level, num_channels);
	ERR_FAIL_COND_MSG(!refiner, "Refiner couldn't be created, numVertsPerFace array likely lost.");
	_create_subdivision_vertices(refiner, p_level, face_varying_data);
	_create_subdivision_faces(refiner, p_level, face_varying_data);
	//free memory
	delete refiner;

	if (calculate_normals) {
		topology_data.normal_array = _calculate_smooth_normals(topology_data.vertex_array, topology_data.index_array);
	}
}

//TODO: virtual calls are not implemented yet in godot cpp (I think, it wasnt 2 weeks ago and didn't see any commit)
OpenSubdiv::Sdc::SchemeType Subdivider::_get_refiner_type() const {
	return Sdc::SchemeType::SCHEME_CATMARK;
}
void Subdivider::_create_subdivision_faces(OpenSubdiv::Far::TopologyRefiner *refiner,
		const int32_t p_level, const bool face_varying_data) {
}
PackedVector3Array Subdivider::_calculate_smooth_normals(const PackedVector3Array &quad_vertex_array, const PackedInt32Array &quad_index_array) {
	return PackedVector3Array();
}
Array Subdivider::_get_triangle_arrays() const {
	return Array();
}

Vector<int> Subdivider::_get_face_vertex_count() const {
	return Vector<int>();
}
Array Subdivider::_get_direct_triangle_arrays() const {
	return Array();
}

void Subdivider::_bind_methods() {
	ClassDB::bind_method("get_subdivided_arrays", &Subdivider::get_subdivided_arrays);
	ClassDB::bind_method("get_subdivided_topology_arrays", &Subdivider::get_subdivided_topology_arrays);
}

Subdivider::Subdivider() {
}
Subdivider::~Subdivider() {
}
