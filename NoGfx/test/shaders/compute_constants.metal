#include <metal_stdlib>

using namespace metal;

constant float computeScale [[function_constant(0)]];

[[host_name("main")]] kernel void computeMain() {
	if (computeScale > 0.0f) {
	}
}