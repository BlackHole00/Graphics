package main

import "core:log"
import "core:mem"
import "core:os"
import CF "shared:darwodin/CoreFoundation"
import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

check_things :: proc() {
	device := MTL.CreateSystemDefaultDevice()
	defer device->release()

	err: ^NS.Error

	comp_desc := MTL.MTL4CompilerDescriptor.new()
	comp := device->newCompilerWithDescriptor(comp_desc, &err)

	shader_ir, shader_ir_res := os.read_entire_file("res/shaders.metallib", context.allocator)
	if shader_ir_res != nil {
		return
	}

	data := CF.dispatch_data_create(raw_data(shader_ir), len(shader_ir), CF.dispatch_get_current_queue(), nil)
	// device->newLibraryWithData()

	lib := device->newLibraryWithData(data, &err)
	if (lib == nil) {
		return
	}

	value := 10

	consts := MTL.FunctionConstantValues.new()
	consts->setConstantValue_type_atIndex(&value, MTL.DataType.Struct, 0)
	// func := lib->newFunctionWithName_constantValues_error(c_AT("v_main"), nil, &err)

	func_desc := MTL.MTL4LibraryFunctionDescriptor.new()
	func_desc->setName(NS.STR("v_main"))
	func_desc->setLibrary(lib)
	func_desc->setConstantValues(consts)

	// comp->renderPipe
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

