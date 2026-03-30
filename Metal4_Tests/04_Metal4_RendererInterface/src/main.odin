package main

import "base:runtime"
import "core:log"
import "core:mem"
import "core:sync"
import "core:mem/tlsf"
import "core:prof/spall"
import vmem "core:mem/virtual"

main :: proc() {
	context.logger = log.create_console_logger()
	defer log.destroy_console_logger(context.logger)

	// spall_ok: bool
	// spall_ctx, spall_ok = spall.context_create("prof.spall")
	// defer spall.context_destroy(&spall_ctx)
	// log.assertf(spall_ok, "Could not create a spall context. Aborting.")

	// buffer_backing := make([]u8, spall.BUFFER_DEFAULT_SIZE)
	// defer delete(buffer_backing)

	// spall_buffer = spall.buffer_create(buffer_backing, u32(sync.current_thread_id()))
	// defer spall.buffer_destroy(&spall_ctx, &spall_buffer)

	temp_arena: vmem.Arena
	if err := vmem.arena_init_growing(&temp_arena); err != .None {
		log.panicf("Could not create an arena for the temp allocator. Got error `%v` . Aborting.", err)
	}
	defer vmem.arena_destroy(&temp_arena)
	context.temp_allocator = vmem.arena_allocator(&temp_arena)

	tlsf_allocator: tlsf.Allocator
	if err := tlsf.init_from_allocator(&tlsf_allocator, context.allocator, 64 * mem.Megabyte); err != .None {
		log.panicf("Could not create a tlsf allocator for the main allocator. Got error `%v`. Aborting", err)
	}
	defer tlsf.destroy(&tlsf_allocator)
	context.allocator = tlsf.allocator(&tlsf_allocator)

	tracking_allocator: mem.Tracking_Allocator
	mem.tracking_allocator_init(&tracking_allocator, context.allocator)
	defer mem.tracking_allocator_destroy(&tracking_allocator)

	context.allocator = mem.tracking_allocator(&tracking_allocator)

	p_init()
	p_loop()
	p_fini()

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

// spall_ctx: spall.Context
// @(thread_local) spall_buffer: spall.Buffer

// @(instrumentation_enter)
// spall_enter :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
// 	spall._buffer_begin(&spall_ctx, &spall_buffer, "", "", loc)
// }

// @(instrumentation_exit)
// spall_exit :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
// 	spall._buffer_end(&spall_ctx, &spall_buffer)
// }

