#include <metal_stdlib>

struct C_Vertex {
	packed_float3 position;
	packed_float2 uv;
};

struct F_In {
	float4 position [[position]];
	float2 uv;
};
using V_Out = F_In;


struct C_TextureRef {
	metal::texture2d<float> texture;
};

struct C_Resources {
	constant C_TextureRef* textures;
};

struct C_Draw_Instance {
	constant C_Vertex* vertices;
	constant uint32_t* indices;

	metal::float4x4 model;
	metal::float4x4 view;
	metal::float4x4 proj;

	uint32_t texture_selection;
};
static_assert(sizeof(C_Draw_Instance) == 16 + 3 * 16 * 4 + 16);

vertex V_Out v_main(
	uint vertex_id [[vertex_id]],
	constant C_Draw_Instance& instance [[buffer(0)]]
) {
	constant uint32_t* indices  = instance.indices;
	constant C_Vertex* vertices = instance.vertices;
	constant C_Vertex& current_vertex = vertices[indices[vertex_id]];

	V_Out out;
	out.position	= instance.proj * instance.view * instance.model * float4(current_vertex.position, 1.0);
	// out.position	= instance.model * float4(current_vertex.position, 1.0);
	out.uv		= current_vertex.uv;

	return out;
}


fragment float4 f_main(
	F_In in [[stage_in]],
	constant C_Draw_Instance& instance [[buffer(0)]],
	constant C_Resources& resources [[buffer(1)]]
) {
	constexpr metal::sampler s(metal::filter::nearest);
	constant metal::texture2d<float>& texture = resources.textures[instance.texture_selection].texture;

	float4 sample = texture.sample(s, in.uv);

	return sample;
}
