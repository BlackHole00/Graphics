#include <metal_stdlib>

using namespace metal;

constant float fragmentScale [[function_constant(0)]];

[[host_name("main")]] fragment float4 fragmentMain() {
	return float4(fragmentScale, 0.0, 0.0, 1.0);
}