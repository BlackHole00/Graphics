#include <metal_stdlib>

struct C_Vertex {
	float3 position;
	float3 color;
};

struct F_In {
	float4 position [[position]];
	float4 color;
};
using V_Out = F_In;


vertex V_Out v_main(
	uint vertex_id [[vertex_id]],
	constant C_Vertex* vertices [[buffer(0)]]
) {
	V_Out out;
	out.position	= float4(vertices[vertex_id].position, 1.0);
	out.color	= float4(vertices[vertex_id].color,    1.0);

	return out;
}


fragment float4 f_main(
	F_In in [[stage_in]]
) {
	return in.color;
}
