package main

import "core:log"
import "core:mem"
import MTL "shared:darwodin/Metal"

check_things :: proc() {
	device := MTL.CreateSystemDefaultDevice()
	defer device->release()

	buffer := device->newBufferWithLength(64, { .CPUCacheModeWriteCombined })
	defer buffer->release()

	cpu_ptr := buffer->contents()
	gpu_ptr := buffer->gpuAddress()

	log.infof("%64b %64b %64b", cpu_ptr, cast(rawptr)cast(uintptr)gpu_ptr, cast(uintptr)cpu_ptr ~ cast(uintptr)gpu_ptr)

	buffer2 := device->newBufferWithLength_options(64, { .CPUCacheModeWriteCombined })
	defer buffer2->release()

	data := (cast(^u32)buffer2->contents())^
}

main :: proc() {
	context.logger = log.create_console_logger()
	defer log.destroy_console_logger(context.logger)

	tracking_allocator: mem.Tracking_Allocator
	mem.tracking_allocator_init(&tracking_allocator, context.allocator)
	defer mem.tracking_allocator_destroy(&tracking_allocator)

	context.allocator = mem.tracking_allocator(&tracking_allocator)

	check_things()

	c_print_tracking_allocator_stats(&tracking_allocator)
}

c_print_tracking_allocator_stats :: proc(tracking_allocator: ^mem.Tracking_Allocator) {
	log.info("Main context allocator stats:")
	log.info("\t- total_memory_allocated", tracking_allocator.total_memory_allocated)
	log.info("\t- total_allocation_count", tracking_allocator.total_allocation_count)
	log.info("\t- total_memory_freed", tracking_allocator.total_memory_freed)
	log.info("\t- total_free_count", tracking_allocator.total_free_count)
	log.info("\t- peak_memory_allocated", tracking_allocator.peak_memory_allocated)

	if len(tracking_allocator.bad_free_array) > 0 {
		log.info("\t- Bad frees:")
		for bad_free in tracking_allocator.bad_free_array {
			log.info("\t\t- %p: %v", bad_free.memory, bad_free.location)
		}
	} else {
		log.info("\t- No bad frees")
	}

	if len(tracking_allocator.allocation_map) > 0 {
		log.info("\t- Leaks:")
		for ptr, entry in tracking_allocator.allocation_map {
			log.infof("\t\t- %p (%d bytes): %v", ptr, entry.size, entry.location)
		}
	} else {
		log.info("\t- No leaks")
	}
}

