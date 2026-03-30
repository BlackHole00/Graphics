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

vertex V_Out v_main(
	uint vertex_id [[vertex_id]],
	constant C_Vertex* vertices [[buffer(0)]],
	constant uint32_t* indices [[buffer(1)]]
) {
	constant C_Vertex& current_vertex = vertices[indices[vertex_id]];

	V_Out out;
	out.position	= float4(current_vertex.position, 1.0);
	out.uv		= current_vertex.uv;

	return out;
}


fragment float4 f_main(
	F_In in [[stage_in]],
	constant uint32_t& texture_selection [[buffer(2)]],
	constant C_Resources& resources [[buffer(3)]]
) {
	constexpr metal::sampler s(metal::filter::nearest);
	constant metal::texture2d<float>& texture = resources.textures[texture_selection].texture;

	float4 sample = texture.sample(s, in.uv);

	return sample;
}
