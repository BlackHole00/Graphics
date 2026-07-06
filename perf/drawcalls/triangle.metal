#include <metal_stdlib>
using namespace metal;

typedef packed_float2 Vertex;

struct VertexOut {
	float4 position [[position]];
};

struct Args {
	device	Vertex*		vertices;
		packed_float2	position;
};

constant float width [[function_constant(0)]];
constant float height [[function_constant(1)]];

vertex VertexOut vertexMain(
		uint		vertexIndex	[[vertex_id]],
		uint		instanceId	[[instance_id]],
	device	const Args&	args		[[buffer(0)]]
) {
	float2 localPosition = args.vertices[vertexIndex].xy;

	float2 worldPosition = localPosition + args.position.xy;
	float2 clipPosition = float2(
		(worldPosition.x / width) * 2.0 - 1.0,
		1.0 - (worldPosition.y / height) * 2.0);

	VertexOut vertexOut;
	vertexOut.position = float4(clipPosition, 0.0, 1.0);
	return vertexOut;
}


fragment float4 fragmentMain() {
	return float4(1.0, 1.0, 1.0, 1.0);
}



