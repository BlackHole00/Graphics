package main

import "core:fmt"
import "core:log"
import "core:mem"
import "core:time"
import AK "shared:darwodin/AppKit"
import CG "shared:darwodin/CoreGraphics"
import MTL "shared:darwodin/Metal"
import NS "shared:darwodin/Foundation"
import CA "shared:darwodin/QuartzCore"

R_NUM_FRAMES_IN_FLIGHT :: 3

R_Error :: enum {
	Out_Of_Gpu_Memory,
	Resource_Creation_Failed,
	Out_Of_Resource_Slots,
	Could_Not_Open_Resource_File,
	Incongruent_Resource_Description,
	Invalid_Resource,
}
R_Result :: union #shared_nil {
	R_Error,
}


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
	last_view_size:		CG.Size,

	shader_library:		^MTL.Library,
	pipeline_compiler:	^MTL.MTL4Compiler,
	render_pipeline:	^MTL.RenderPipelineState,
	depth_stencil_state:	^MTL.DepthStencilState,

	present_pipeline:	^MTL.RenderPipelineState,
	present_argument_table:	^MTL.MTL4ArgumentTable,

	clear_color:		MTL.ClearColor,
	command_allocators:	[R_NUM_FRAMES_IN_FLIGHT]^MTL.MTL4CommandAllocator,

	frame_heaps:		[R_NUM_FRAMES_IN_FLIGHT]^MTL.Heap,
	frame_arenas:		[R_NUM_FRAMES_IN_FLIGHT]R_Arena,
	frame_residency_set:	^MTL.ResidencySet,

	free_to_present_fence:	^MTL.Fence,
	main_rendertargets:	[R_NUM_FRAMES_IN_FLIGHT]R_Rendertarget,

	argument_table:		^MTL.MTL4ArgumentTable,

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

	for &alloc in r_renderer.command_allocators {
		alloc = r_renderer.device->newCommandAllocator()
	}

	for &heap in r_renderer.frame_heaps {
		heap_descriptor := MTL.HeapDescriptor.new()->autorelease()
		heap_descriptor->setSize(128 * mem.Megabyte)
		heap_descriptor->setStorageMode(.Shared)
		heap_descriptor->setHazardTrackingMode(.Untracked)
		heap_descriptor->setResourceOptions({})

		heap = r_renderer.device->newHeapWithDescriptor(heap_descriptor)
	}

	for &arena, i in r_renderer.frame_arenas {
		r_arena_create(&arena, 64 * mem.Megabyte, r_renderer.frame_heaps[i])
	}

	for &rendertarget, i in r_renderer.main_rendertargets {
		r_rendetarget_create(
			&rendertarget,
			{
				cast(uint)r_renderer.last_view_size.width,
				cast(uint)r_renderer.last_view_size.height,
			},
			r_renderer.frame_heaps[i],
		)
	}

	r_renderer.queue = r_renderer.device->newMTL4CommandQueue()
	log.assert(r_renderer.queue != nil, "Could not create a Command Queue. Aborting.")

	r_renderer.render_pipeline = r_make_renderpipeline(.RGBA8Unorm)

	r_renderer.present_pipeline = r_make_presentpipeline(auto_cast r_renderer.target_view_layer->pixelFormat())
	argument_table_desc := MTL.MTL4ArgumentTableDescriptor.new()->autorelease()
	argument_table_desc->setMaxTextureBindCount(1)
	r_renderer.present_argument_table = r_renderer.device->newArgumentTableWithDescriptor(argument_table_desc, nil)

	r_renderer.depth_stencil_state = r_make_depth_stencil_state()

	r_init_resource_storage()
	r_register_texture_from_file("res/Bricks.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Dirt.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/HappyGrass.png",	.RGBA8Unorm, true)
	r_register_texture_from_file("res/Stone.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Sand.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/Painting.png",	.RGBA8Unorm, true)
	r_register_texture_from_file("res/Scenery.png",		.RGBA8Unorm, true)
	r_register_texture_from_file("res/OpenGL.png",		.RGBA8Unorm, true)

	residency_set_descriptor := MTL.ResidencySetDescriptor.new()->autorelease()
	residency_set_descriptor->setInitialCapacity(1)
	r_renderer.frame_residency_set = r_renderer.device->newResidencySetWithDescriptor(residency_set_descriptor, nil)

	r_renderer.queue->addResidencySet(auto_cast r_renderer.target_view_layer->residencySet())
	r_renderer.queue->addResidencySet(r_resources.residency_set)
	r_renderer.queue->addResidencySet(r_renderer.frame_residency_set)

	r_renderer.argument_table = r_make_argument_table(2)

	r_renderer.clear_color = { 0.1, 0.2, 0.3, 1.0 }

	r_renderer.frame_end_event = r_renderer.device->newSharedEvent()
	r_renderer.frame_end_event->setLabel(c_AT("r_renderer.frame_end_event"))
	r_renderer.start_time = time.tick_now()
}

r_make_depth_stencil_state :: proc() -> ^MTL.DepthStencilState {
	descriptor := MTL.DepthStencilDescriptor.new()->autorelease()
	descriptor->setDepthCompareFunction(.Less)
	descriptor->setDepthWriteEnabled(true)
	
	return r_renderer.device->newDepthStencilStateWithDescriptor(descriptor)
}

r_prepare_frame :: proc() -> bool {
	r_renderer.frame_drawable = r_renderer.target_view_layer->nextDrawable()
	if r_renderer.frame_drawable == nil {
		log.info("Skipping")
		return false;
	}

	r_wait_for_previous_frame()
	r_update_timings()

	if r_renderer.free_to_present_fence != nil {
		r_renderer.free_to_present_fence->release()
	}
	r_renderer.free_to_present_fence = r_renderer.device->newFence()

	bounds := r_renderer.target_view->bounds()
	scale  := r_renderer.target_view->window()->backingScaleFactor()
	size   := CG.Size {
		bounds.size.width  * scale,
		bounds.size.height * scale,
	}
	size_uint := [2]uint{
		cast(uint)size.width,
		cast(uint)size.height,
	}

	if r_renderer.last_view_size != size {
		r_renderer.target_view_layer->setFrame(auto_cast bounds)
		r_renderer.target_view_layer->setDrawableSize(size)
	}

	if r_current_main_rendertarget().size != size_uint {
		r_rendertarget_resize(r_current_main_rendertarget(), size_uint, r_current_heap())
	}

	frame_allocator := r_current_command_allocator()
	frame_allocator->reset()

	current_heap, previous_heap := r_current_heap(), r_previous_heap()
	r_renderer.frame_residency_set->removeAllocation(previous_heap)
	r_renderer.frame_residency_set->addAllocation(current_heap)
	r_renderer.frame_residency_set->commit()

	current_arena  := r_current_arena()
	r_arena_reset(current_arena)

	r_renderer.frame_command_buffer = r_renderer.device->newCommandBuffer()->autorelease()
	r_renderer.frame_command_buffer->beginCommandBufferWithAllocator(frame_allocator)

	r_generate_mipmaps_for_textures()

	return true
}

r_blit_to_target_view :: proc() {
	presentation_desc := MTL.MTL4RenderPassDescriptor.new()->autorelease()
	color_attachment := presentation_desc->colorAttachments()->objectAtIndexedSubscript(0)
	color_attachment->setTexture(auto_cast r_renderer.frame_drawable->texture())
	color_attachment->setLoadAction(.DontCare)
	color_attachment->setStoreAction(.Store)

	presentation := r_renderer.frame_command_buffer->renderCommandEncoderWithDescriptor(presentation_desc)

	presentation->waitForFence(r_renderer.free_to_present_fence, .StageFragment)

	r_renderer.present_argument_table->setTexture(r_current_main_rendertarget().color_attachment->gpuResourceID(), 0)
	presentation->setRenderPipelineState(r_renderer.present_pipeline)
	presentation->setArgumentTable(r_renderer.present_argument_table, { .StageFragment })
	presentation->drawPrimitives(.Triangle, 0, 3)
	presentation->endEncoding()
	
}

r_end_frame :: proc() {
	r_blit_to_target_view()

	r_renderer.frame_command_buffer->endCommandBuffer()
	r_present()

	r_signal_frame_end()
	r_renderer.current_frame += 1
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
	depth_attachment := descriptor->depthAttachment()

	color_attachment->setClearColor(r_renderer.clear_color)
	color_attachment->setTexture(r_current_main_rendertarget().color_attachment)
	color_attachment->setLoadAction(.Clear)
	color_attachment->setStoreAction(.Store)

	depth_attachment->setClearDepth(1.0)
	depth_attachment->setTexture(r_current_main_rendertarget().depth_attachment)


	return descriptor
}

r_current_command_allocator :: proc() -> ^MTL.MTL4CommandAllocator {
	return r_renderer.command_allocators[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]
}

r_current_heap :: proc() -> ^MTL.Heap {
return r_renderer.frame_heaps[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]
}

r_previous_heap :: proc() -> ^MTL.Heap {
	return r_renderer.frame_heaps[(r_renderer.current_frame - 1) % R_NUM_FRAMES_IN_FLIGHT]
}

r_current_arena :: proc() -> ^R_Arena {
	return &r_renderer.frame_arenas[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]
}

r_current_main_rendertarget :: proc() -> ^R_Rendertarget {
	return &r_renderer.main_rendertargets[r_renderer.current_frame % R_NUM_FRAMES_IN_FLIGHT]
}

r_init_layer_for_target_view :: proc() {
	r_renderer.target_view_layer = CA.MetalLayer.new()
	r_renderer.target_view_layer->setFramebufferOnly(false)
	r_renderer.target_view_layer->setDevice(auto_cast r_renderer.device)
	r_renderer.target_view_layer->setAutoresizingMask({ .kCALayerWidthSizable, .kCALayerHeightSizable })
	r_renderer.target_view_layer->setFrame(auto_cast r_renderer.target_view->bounds())

	r_renderer.target_view->setWantsLayer(true)
	r_renderer.target_view->setLayer(r_renderer.target_view_layer)

	bounds := r_renderer.target_view->bounds()
	scale  := r_renderer.target_view->window()->backingScaleFactor()
	r_renderer.last_view_size = {
		bounds.size.width  * scale,
		bounds.size.height * scale,
	}

	r_renderer.target_view_layer->setFrame(auto_cast bounds)
	r_renderer.target_view_layer->setDrawableSize(r_renderer.last_view_size)
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

r_make_argument_table :: proc(buffer_count: int) -> ^MTL.MTL4ArgumentTable {
	descriptor := MTL.MTL4ArgumentTableDescriptor.new()->autorelease()
	descriptor->setMaxBufferBindCount(cast(NS.UInteger)buffer_count)

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

r_make_presentpipeline :: proc(pixel_format: MTL.PixelFormat) -> ^MTL.RenderPipelineState {

	vertex_function := MTL.MTL4LibraryFunctionDescriptor.new()->autorelease()
	vertex_function->setLibrary(r_renderer.shader_library)
	vertex_function->setName(c_AT("blit_vertex_main"))

	fragment_function := MTL.MTL4LibraryFunctionDescriptor.new()->autorelease()
	fragment_function->setLibrary(r_renderer.shader_library)
	fragment_function->setName(c_AT("blit_fragment_main"))

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
		"Could not create the blit pipeline state. Got error `%v`. Aborting.",
		error->localizedDescription()->UTF8String(),
	)

	return pipeline
}
