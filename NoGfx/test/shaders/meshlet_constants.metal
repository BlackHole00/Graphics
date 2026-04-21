#include <metal_stdlib>

using namespace metal;

constant uint meshletPrimitiveCount [[function_constant(0)]];

[[host_name("main")]] [[mesh]] void meshletMain(mesh<float4, void, 1, 1, topology::triangle> out) {
	out.set_primitive_count(meshletPrimitiveCount);
}