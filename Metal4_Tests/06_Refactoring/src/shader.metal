#include <metal_stdlib>


struct C_Vertex {
	packed_float3 position;
	packed_float2 uv;
};

struct C_Mesh {
	constant C_Vertex* vertices;
	constant uint32_t* indices;
};

enum C_Material_Texture {
	C_MATERIAL_BASE_COLOR = 0,
	C_MATERIAL_METALLIC,
	C_MATERIAL_ROUGHNESS,
	C_MATERIAL_NORMAL,
	C_MATERIAL_AMBIENT_OCCLUSION,
	C_MATERIAL_COUNT,
};

struct C_Material {
	metal::array<metal::texture2d<float>, C_MATERIAL_COUNT> textures;
};


struct F_In {
	float4 position [[position]];
	float2 uv;
	uint  instance_id;
};
using V_Out = F_In;


struct C_Draw_Instance {
	metal::float4x4		model;
	constant C_Mesh&	mesh;
	constant C_Material&	material;
};

struct C_Camera_Data {
	metal::float4x4		projection;
	metal::float4x4		view;
};


vertex V_Out v_main(
	uint vertex_id [[vertex_id]],
	uint instance_id [[instance_id]],
	constant C_Camera_Data& camera [[buffer(0)]],
	constant C_Draw_Instance* instances [[buffer(1)]]
) {
	constant C_Draw_Instance& instance = instances[instance_id];
	constant C_Mesh& mesh = instance.mesh;

	constant uint32_t* indices  = mesh.indices;
	constant C_Vertex* vertices = mesh.vertices;

	constant C_Vertex& current_vertex = vertices[indices[vertex_id]];

	V_Out out;
	out.position	= camera.projection * camera.view * instance.model * float4(current_vertex.position, 1.0);
	// out.position	= float4(current_vertex.position, 1.0);
	out.uv		= current_vertex.uv;
	out.instance_id = instance_id;

	return out;
}


fragment float4 f_main(
	F_In in [[stage_in]],
	constant C_Draw_Instance* instances [[buffer(1)]]
) {
	constant C_Draw_Instance& instance = instances[in.instance_id];
	constant C_Material& material = instance.material;

	constexpr metal::sampler s(metal::filter::nearest, metal::mag_filter::linear, metal::min_filter::linear);
	constant metal::texture2d<float>& texture = material.textures[C_MATERIAL_BASE_COLOR];

	float4 sample = texture.sample(s, in.uv);

	return sample;
}
