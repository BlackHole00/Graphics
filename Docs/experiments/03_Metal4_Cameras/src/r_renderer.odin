package main

import "core:fmt"
import "core:log"
import la "core:math/linalg"
import "core:time"
import "vendor:glfw"
import AK "shared:darwodin/AppKit"
import MTL "shared:darwodin/Metal"
import NS "shared:darwodin/Foundation"
import CA "shared:darwodin/QuartzCore"

R_NUM_FRAMES_IN_FLIGHT :: 3

C_Draw_Instance :: struct #packed {
	vertices:	MTL.GPUAddress,
	indices:	MTL.GPUAddress,
	model:		matrix[4,4]f32,
	view:		matrix[4,4]f32,
	proj:		matrix[4,4]f32,
	texture_index:	u32,
	_pad:		[4 + 8]byte,
};
#assert(size_of(C_Draw_Instance) == 16 + 3 * 16 * 4 + 16)


R_Renderer :: struct {
	start_time:		time.Tick,
	last_frame_time:	time.Tick,
	average_ms:		f64,
	average_fps:		f64,

	target_view:		^AK.View,
	target_view_layer:	^CA.MetalLayer,
	current_frame:		u64,
	frame_drawable:		^CA.MetalDrawable,
	frame_end_event:	^MTL.SharedEvent,

	shader_library:		^MTL.Library,
	pipeline_compiler:	^MTL.MTL4Compiler,
	render_pipeline:	^MTL.RenderPipelineState,

	clear_color:		MTL.ClearColor,
	vertex_buffer:		^MTL.Buffer,
	index_buffer:		^MTL.Buffer,
	draw_instance_buffer:	^MTL.Buffer,
	argument_table:		^MTL.MTL4ArgumentTable,
	residency_set:		^MTL.ResidencySet,
	command_allocators:	[R_NUM_FRAMES_IN_FLIGHT]^MTL.MTL4CommandAllocator,

	camera:			R_Camera,

	frame_command_buffer:	^MTL.MTL4CommandBuffer,

	device:	^MTL.Device,
	queue:	^MTL.MTL4CommandQueue,
}
r_renderer: R_Renderer

r_init :: proc(target_view: ^AK.View) {
	c_scoped_autoreleasepool()

	r_renderer.target_view = target_view
	r_renderer.target_view_layer = cast(^CA.MetalLayer)target_view

	r_renderer.device = MTL.CreateSystemDefaultDevice()
	log.assert(r_renderer.device != nil, "Could not create the Metal device. Aborting.")
	if !r_renderer.device->supportsFamily(.Metal4) {
		log.panic("Sorry, the renderer requires Metal 4. Aborting.")
	}

	r_init_layer_for_target_view()

	r_init_shader_stuff()

	r_renderer.camera.position = { 0.0, 0.0, 0.0 }
	r_renderer.camera.rotation = { 0, la.to_radians(f32(90.0)), 0 }
	r_renderer.camera.fov = 90
	r_renderer.camera.near = 0.001
	r_renderer.camera.far = 100.0
	r_renderer.camera.aspect = 640 / 480

	r_renderer.vertex_buffer, r_renderer.index_buffer = r_make_quad_buffers()
	r_renderer.draw_instance_buffer = r_make_draw_instance()

	// r_prepare_texture_select_buffer()
	r_renderer.residency_set  = r_make_residency_set()
	r_renderer.argument_table = r_make_argument_table()

	for &alloc in r_renderer.command_allocators {
		alloc = r_renderer.device->newCommandAllocator()
	}

	r_renderer.queue = r_renderer.device->newMTL4CommandQueue()
	log.assert(r_renderer.queue != nil, "Could not create a Command Queue. Aborting.")

	r_renderer.render_pipeline = r_make_renderpipeline(auto_cast r_renderer.target_view_layer->pixelFormat())

	r_init_resource_storage()
	r_register_texture_from_file("res/Bricks.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Dirt.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/HappyGrass.png",	.RGBA8Unorm, true)
	r_register_texture_from_file("res/Stone.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Sand.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Painting.png",	.RGBA8Unorm, true)
	r_register_texture_from_file("res/Scenery.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/OpenGL.png",		.RGBA8Unorm, true)
	r_sync_resource_storage_gpu_references()

	r_renderer.argument_table->setAddress(r_renderer.draw_instance_buffer->gpuAddress(), 0)
	r_renderer.argument_table->setAddress(r_resources.gpu_references->gpuAddress(), 1)

	r_renderer.residency_set->addAllocation(r_renderer.vertex_buffer)
	r_renderer.residency_set->addAllocation(r_renderer.index_buffer)
	r_renderer.residency_set->addAllocation(r_renderer.draw_instance_buffer)
	r_renderer.residency_set->addAllocation(r_resources.heap)
	r_renderer.residency_set->commit()
	r_renderer.queue->addResidencySet(auto_cast r_renderer.target_view_layer->residencySet())
	r_renderer.queue->addResidencySet(r_renderer.residency_set)

	r_renderer.clear_color = { 0.1, 0.2, 0.3, 1.0 }

	r_renderer.frame_end_event = r_renderer.device->newSharedEvent()
	r_renderer.start_time = time.tick_now()
}

r_setup_frame_texture :: proc() {
}

r_make_draw_instance :: proc() -> ^MTL.Buffer {
	instance := r_renderer.device->newBufferWithLength(
		size_of(C_Draw_Instance) * R_NUM_FRAMES_IN_FLIGHT,
		{}
	)

	return instance
}

r_render :: proc() {
	c_scoped_autoreleasepool()
	
	r_renderer.frame_drawable = r_renderer.target_view_layer->nextDrawable()
	if r_renderer.frame_drawable == nil {
		log.info("Skipping")
		return;
	}

	r_wait_for_previous_frame()
	r_update_timings()

	r_setup_frame_texture()

	bounds := r_renderer.target_view->bounds()
	scale  := r_renderer.target_view->window()->backingScaleFactor()

	r_renderer.target_view_layer->setFrame(auto_cast bounds)
	r_renderer.target_view_layer->setDrawableSize({
		bounds.size.width  * scale,
		bounds.size.height * scale,
	})

	frame_allocator := r_current_command_allocator()
	frame_allocator->reset()

	r_renderer.frame_command_buffer = r_renderer.device->newCommandBuffer()->autorelease()
	r_renderer.frame_command_buffer->beginCommandBufferWithAllocator(frame_allocator)
	r_do_main_renderpass()
	r_renderer.frame_command_buffer->endCommandBuffer()
	
	r_present()

	r_signal_frame_end()
	r_renderer.current_frame += 1
}

r_prepare_draw_instance_buffer :: proc() {
	draw_instances := cast([^]C_Draw_Instance)r_renderer.draw_instance_buffer->contents()
	prev_instance := &draw_instances[(r_renderer.current_frame - 1) % R_NUM_FRAMES_IN_FLIGHT]
	current_instance := &draw_instances[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]

	{ // texture selection
		previous_selection := prev_instance.texture_index - 1

		new_selection := previous_selection
		key := glfw.KEY_1
		for {
			if key > glfw.KEY_9 {
				break
			}

			if .Just_Pressed in p_get_keystate(key) {
				new_selection = cast(u32)(key - glfw.KEY_1)
				break
			}

			key += 1
		}

		current_instance.texture_index = new_selection + 1
	}

	// r_renderer.camera.rotation = { 0, la.to_radians(f32(90.0) + 1 * cast(f32)r_renderer.current_frame), 0 }

	current_instance.vertices = r_renderer.vertex_buffer->gpuAddress()
	current_instance.indices = r_renderer.index_buffer->gpuAddress()
	current_instance.model =
		la.matrix4_translate([3]f32{ 2, 0, 2 }) *
		la.matrix4_rotate(la.to_radians(cast(f32)r_renderer.current_frame), [3]f32{ 0, 1, 0})
	current_instance.view = r_camera_view_matrix(r_renderer.camera)
	current_instance.proj = r_camera_proj_matrix(r_renderer.camera)
}

r_do_main_renderpass :: proc() {
	r_prepare_draw_instance_buffer()
	r_renderer.argument_table->setAddress(
		r_renderer.draw_instance_buffer->gpuAddress() +
			cast(MTL.GPUAddress)((r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT) * size_of(C_Draw_Instance)),
		// r_renderer.draw_instance_buffer->gpuAddress(),
		0,
	)

	renderpass := r_renderer.frame_command_buffer->renderCommandEncoderWithDescriptor(
		r_get_renderpassdescriptor_for_target_view())
	renderpass->setRenderPipelineState(r_renderer.render_pipeline)
	renderpass->setArgumentTable(r_renderer.argument_table, { .StageVertex, .StageFragment })
	renderpass->drawPrimitives(.Triangle, 0, 6)
	renderpass->endEncoding()
}

r_present :: proc() {
	frame_drawable := cast(^MTL.Drawable)r_renderer.frame_drawable

	r_renderer.queue->waitForDrawable(frame_drawable)
	r_renderer.queue->commit(&r_renderer.frame_command_buffer, 1)
	r_renderer.queue->signalDrawable(frame_drawable)
	frame_drawable->present()
}

r_fini :: proc() {
	r_renderer.target_view_layer->release()
}

r_update_timings :: proc() {
	time_now := time.tick_now()
	frame_duration := time.tick_diff(r_renderer.last_frame_time, time_now)

	frame_ms := time.duration_milliseconds(frame_duration)
	frame_fps := 1000.0 / frame_ms

	r_renderer.average_ms = 0.2 * r_renderer.average_ms  + 0.8 * frame_ms
	r_renderer.average_fps = 0.2 * r_renderer.average_fps + 0.8 * frame_fps
	r_renderer.last_frame_time = time_now
	
	if r_renderer.current_frame % 60 == 0 {
		window_title := fmt.tprintf("Metal [%fms - %ffps]", r_renderer.average_ms, r_renderer.average_fps)
		p_set_window_title(window_title)
	}
}

r_wait_for_previous_frame :: proc() {
	if r_renderer.current_frame < R_NUM_FRAMES_IN_FLIGHT {
		return
	}

	for {
		before_timeout := r_renderer.frame_end_event->waitUntilSignaledValue(
			r_renderer.current_frame - R_NUM_FRAMES_IN_FLIGHT,
			20
		)

		if before_timeout {
			break
		} else {
			log.warn("The frame %d is late!", r_renderer.current_frame)
		}
	}
}

r_signal_frame_end :: proc() {
	r_renderer.frame_end_event->setSignaledValue(r_renderer.current_frame)
	r_renderer.queue->signalEvent(r_renderer.frame_end_event, r_renderer.current_frame)
}

r_get_renderpassdescriptor_for_target_view :: proc() -> ^MTL.MTL4RenderPassDescriptor {
	descriptor := MTL.MTL4RenderPassDescriptor.new()->autorelease()
	color_attachment := descriptor->colorAttachments()->objectAtIndexedSubscript(0)

	color_attachment->setClearColor(r_renderer.clear_color)
	color_attachment->setTexture(auto_cast r_renderer.frame_drawable->texture())
	color_attachment->setLoadAction(.Clear)
	color_attachment->setStoreAction(.Store)

	return descriptor
}

r_current_command_allocator :: proc() -> ^MTL.MTL4CommandAllocator {
	return r_renderer.command_allocators[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]
}

r_init_layer_for_target_view :: proc() {
	r_renderer.target_view_layer = CA.MetalLayer.new()
	r_renderer.target_view_layer->setDevice(auto_cast r_renderer.device)
	r_renderer.target_view_layer->setAutoresizingMask({ .kCALayerWidthSizable, .kCALayerHeightSizable })
	r_renderer.target_view_layer->setFrame(auto_cast r_renderer.target_view->bounds())

	r_renderer.target_view->setWantsLayer(true)
	r_renderer.target_view->setLayer(r_renderer.target_view_layer)
}

r_init_shader_stuff :: proc() {
	library_path := c_AT("res/shaders.metallib")
	library_url := NS.URL.alloc()->initFileURLWithPath_isDirectory(
		library_path,
		false,
	)->autorelease()

	error: ^NS.Error
	r_renderer.shader_library = r_renderer.device->newLibraryWithURL(library_url, &error)
	log.assertf(
		r_renderer.shader_library != nil,
		"Could not create a shader library. Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)

	r_renderer.pipeline_compiler = r_renderer.device->newCompilerWithDescriptor(
		MTL.MTL4CompilerDescriptor.new()->autorelease(),
		&error,
	)
	log.assertf(
		r_renderer.pipeline_compiler != nil,
		"Could not create a pipeline compiler. Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)
}

r_make_triangle_vertex_buffer :: proc() -> ^MTL.Buffer {
	vertex_buffer := r_renderer.device->newBufferWithBytes(
		raw_data(R_TRIANGLE_VERTICES[:]),
		size_of(R_TRIANGLE_VERTICES),
		{ .StorageModeManaged, .HazardTrackingModeUntracked },
	)
	
	return vertex_buffer
}

r_make_quad_buffers :: proc() -> (vertex_buffer: ^MTL.Buffer, index_buffer: ^MTL.Buffer) {
	vertex_buffer = r_renderer.device->newBufferWithBytes(
		raw_data(R_QUAD_VERTICES[:]),
		size_of(R_QUAD_VERTICES),
		{},
	)
	index_buffer = r_renderer.device->newBufferWithBytes(
		raw_data(R_QUAD_INDICES[:]),
		size_of(R_QUAD_INDICES),
		{},
	)

	return
}

r_make_residency_set :: proc() -> ^MTL.ResidencySet {
	descriptor := MTL.ResidencySetDescriptor.new()->autorelease()

	error: ^NS.Error
	residency_set := r_renderer.device->newResidencySetWithDescriptor(descriptor, &error)
	log.assertf(
		residency_set != nil,
		"Could not create a residency set. Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)

	return residency_set
}

r_make_argument_table :: proc() -> ^MTL.MTL4ArgumentTable {
	descriptor := MTL.MTL4ArgumentTableDescriptor.new()->autorelease()
	descriptor->setMaxBufferBindCount(3)

	error: ^NS.Error
	argument_table := r_renderer.device->newArgumentTableWithDescriptor(
		descriptor,
		&error,
	)
	log.assertf(
		argument_table != nil,
		"Could not create an argument table Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)

	return argument_table
}

r_make_renderpipeline :: proc(pixel_format: MTL.PixelFormat) -> ^MTL.RenderPipelineState {

	vertex_function := MTL.MTL4LibraryFunctionDescriptor.new()->autorelease()
	vertex_function->setLibrary(r_renderer.shader_library)
	vertex_function->setName(c_AT("v_main"))

	fragment_function := MTL.MTL4LibraryFunctionDescriptor.new()->autorelease()
	fragment_function->setLibrary(r_renderer.shader_library)
	fragment_function->setName(c_AT("f_main"))

	descriptor := MTL.MTL4RenderPipelineDescriptor.new()->autorelease()
	descriptor->colorAttachments()->objectAtIndexedSubscript(0)->setPixelFormat(pixel_format)
	descriptor->setVertexFunctionDescriptor(vertex_function)
	descriptor->setFragmentFunctionDescriptor(fragment_function)

	error: ^NS.Error
	pipeline := r_renderer.pipeline_compiler->newRenderPipelineStateWithDescriptor(
		descriptor,
		nil,
		&error
	)
	log.assertf(
		pipeline != nil,
		"Could not create the render pipeline state. Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)

	return pipeline
}
