package main

import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

R_Arena_Buffer :: struct {
	buffer:		^MTL.Buffer,
	offset:		uintptr,
	length:		uintptr,
}

r_arena_buffer_gpu_address :: proc(buffer: R_Arena_Buffer) -> MTL.GPUAddress {
	return buffer.buffer->gpuAddress() + cast(MTL.GPUAddress)buffer.offset
}

r_arena_buffer_contents :: proc(buffer: R_Arena_Buffer) -> rawptr {
	return cast(rawptr)(cast(uintptr)buffer.buffer->contents() + buffer.offset)
}


R_Arena :: struct {
	buffer:		^MTL.Buffer,
	used:		uintptr,
	length:		uintptr
}

r_arena_create :: proc(arena: ^R_Arena, length: uint, heap: ^MTL.Heap) -> R_Result {
	arena.buffer = heap->newBufferWithLength(
		cast(NS.UInteger)length,
		{},
	)
	if arena.buffer == nil {
		return .Out_Of_Gpu_Memory
	}

	arena.used = 0
	arena.length = cast(uintptr)length

	return nil
}

r_arena_new_buffer :: proc(arena: ^R_Arena, length: uint) -> (buffer: R_Arena_Buffer, result: R_Result) {
	if arena.used + cast(uintptr)length > arena.length {
		return {}, .Out_Of_Gpu_Memory
	}

	// offset := mem.align_forward_uintptr(arena.used, 64)
	
	buffer = R_Arena_Buffer {
		offset = arena.used,
		length = cast(uintptr)length,
		buffer = arena.buffer,
	}

	arena.used += cast(uintptr)length
	return
}

r_arena_reset :: proc(arena: ^R_Arena) {
	arena.used = 0
}

