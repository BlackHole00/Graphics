package main

import "core:log"
import "core:mem"
import MTL "shared:darwodin/Metal"

check_things :: proc() {
	device := MTL.CreateSystemDefaultDevice()
	defer device->release()

	// texture_desc := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(.RGBA8Unorm, 4, 4, false)
	texture_desc := MTL.TextureDescriptor.new()
	texture_desc->setTextureType(._2D)
	texture_desc->setPixelFormat(.RGBA8Unorm)
	texture_desc->setWidth(256)
	texture_desc->setHeight(256)
	texture_desc->setMipmapLevelCount(1)
	texture_desc->setSampleCount(1)

	size_n_align := device->heapTextureSizeAndAlignWithDescriptor(texture_desc)
	log.info(size_n_align)
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

