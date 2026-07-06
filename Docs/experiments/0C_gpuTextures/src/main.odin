package main

import "core:log"
import "core:mem"
import MTL "shared:darwodin/Metal"

check_things :: proc() {
	device := MTL.CreateSystemDefaultDevice()
	defer device->release()

	buffer := device->newBufferWithLength(4 * 4 * 4, {})

	texture_desc := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(.RGBA8Unorm, 4, 4, false)
	texture_desc->setResourceOptions(buffer->resourceOptions())

	texture := buffer->newTextureWithDescriptor(texture_desc, 0, 4 * 4)
	texture_id := texture->gpuResourceID()

	view_desc := MTL.TextureViewDescriptor.new()
	view_desc->setPixelFormat(texture_desc->pixelFormat())
	view_desc->setTextureType(texture_desc->textureType())

	texture_view := texture->newTextureViewWithDescriptor(view_desc)
	texture_view_id := texture_view->gpuResourceID()

	texture_desc2 := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(.R8Unorm, 4, 4, false)
	texture_desc2->setResourceOptions(buffer->resourceOptions())
	texture2 := buffer->newTextureWithDescriptor(texture_desc2, 0, 16)
	texture2_id := texture2->gpuResourceID()

	log.info(texture_id, texture_view_id, texture2_id, texture, texture2)
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

