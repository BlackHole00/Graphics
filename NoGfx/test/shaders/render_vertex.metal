#include <metal_stdlib>

using namespace metal;

struct VertexOut {
	float4 position [[position]];
};

[[host_name("main")]] vertex VertexOut vertexMain(uint vertexId [[vertex_id]]) {
	VertexOut out;
	out.position = float4(float(vertexId), 0.0, 0.0, 1.0);
	return out;
}