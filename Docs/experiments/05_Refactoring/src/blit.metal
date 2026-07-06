#include <metal_stdlib>

using namespace metal;


struct Vertex_Out {
	float4 position [[position]];
	float2 uv;
};
using Fragment_In = Vertex_Out;


Vertex_Out vertex blit_vertex_main(uint vertex_id [[vertex_id]]) {
	const float2 VERTICES[3] = {
		{-1.0, -1.0},
		{ 3.0, -1.0},
		{-1.0,  3.0}
	};

	Vertex_Out output;
	output.position = float4(VERTICES[vertex_id], 0.0, 1.0);
	output.uv = VERTICES[vertex_id] * 0.5 + 0.5;
	output.uv.y *= -1;

	return output;
}


half4 fragment blit_fragment_main(
	Fragment_In input [[stage_in]],
	texture2d<half, access::sample> texture [[texture(0)]]
) {
	constexpr sampler s(address::repeat, filter::linear);
	return texture.sample(s, input.uv);
}

