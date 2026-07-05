#include <metal_stdlib>
using namespace metal;

constant uint width [[function_constant(0)]];
constant uint height [[function_constant(1)]];
constant uint iterations [[function_constant(2)]];

struct DrawArg {
	float time;
};

static float mandelbrot(float2 c, uint maxIterations) {
	float2 z = float2(0.0, 0.0);
	uint i = 0;

	for (; i < maxIterations; ++i) {
		float x = z.x * z.x - z.y * z.y + c.x;
		float y = 2.0 * z.x * z.y + c.y;
		z = float2(x, y);

		if (dot(z, z) > 4.0) {
			break;
		}
	}

	if (i == maxIterations) {
		return 0.0;
	}

	return float(i) / float(maxIterations);
}

[[host_name("main")]]
fragment float4 fragmentMain(
	float4 position [[position]],
	device DrawArg& drawArgs [[buffer(0)]]
) {
	float2 uv = position.xy / float2(float(640), float(480));
	float2 ndc = uv * 2.0 - 1.0;
	float aspect = float(640) / float(480);
	float zoom = 1.5 * (1.0 + 0.3 * sin(drawArgs.time));
	
	// 4x4 antialiasing
	float pixelSize = 1.0 / float(640);
	float value = 0.0;
	for (int sy = 0; sy < 4; ++sy) {
		for (int sx = 0; sx < 4; ++sx) {
			float2 offset = float2(float(sx) - 1.5, float(sy) - 1.5) * pixelSize * 0.25;
			float2 c = float2((ndc.x + offset.x) * aspect, ndc.y + offset.y) * zoom + float2(-0.5, 0.5);
			value += mandelbrot(c, 1024);
		}
	}
	value *= 0.25;

	float3 inside = float3(0.02, 0.02, 0.05);
	float3 outside = float3(0.95, 0.88, 0.35);
	float3 color = mix(inside, outside, value);

	return float4(color, 1.0);
}

