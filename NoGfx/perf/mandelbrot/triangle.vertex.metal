#include <metal_stdlib>
using namespace metal;

typedef packed_float2 Vertex;

struct VertexOut {
	float4 position [[position]];
};

struct Args {
	device	Vertex*		vertices;
};

[[host_name("main")]]
vertex VertexOut vertexMain(
		uint		vertexIndex	[[vertex_id]],
		uint		instanceId	[[instance_id]],
	device	const Args&	args		[[buffer(0)]]
) {
	float2 position = args.vertices[vertexIndex].xy;

	VertexOut vertexOut;
	vertexOut.position = float4(position, 0.0, 1.0);
	return vertexOut;
}


