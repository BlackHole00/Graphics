#include <stdio.h>
#include <strings.h>

#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

#include <rgfw/RGFW.h>

#define c_countof$(_arr) (sizeof(_arr) / sizeof(*_arr))
#define c_zero$(_ptr) memset(_ptr, 0, sizeof(*_ptr))

#define R_NUM_FRAMES_IN_FLIGHT 3

typedef struct R_Vertex {
	float position[3];
	float _pad_0;
	float color[3];
	float _pad_1;
} R_Vertex;

const R_Vertex R_TRIANGLE_VERTICES[3] = {
	{{ -0.5, -0.5, 0.0 }, 0.0, { 1.0, 0.0, 0.0 }, 0.0},
	{{  0.5, -0.5, 0.0 }, 0.0, { 0.0, 1.0, 0.0 }, 0.0},
	{{  0.0,  0.5, 0.0 }, 0.0, { 0.0, 0.0, 1.0 }, 0.0},
};

struct {
	NSView*			target_view;
	CAMetalLayer*		target_view_layer;

	id<CAMetalDrawable>	frame_drawable;
	uint64_t		frame_number;
	id<MTLSharedEvent>	frame_end_event;

	id<MTLDevice>			device;
	id<MTL4CommandQueue>		command_queue;
	id<MTL4CommandAllocator>	command_allocators[R_NUM_FRAMES_IN_FLIGHT];

	id<MTLLibrary>			shader_library;
	id<MTL4Compiler>		pipeline_compiler;
	id<MTLRenderPipelineState>	render_pipeline;

	id<MTLResidencySet>	residency_set;
	id<MTLBuffer>		triangle_vertex_buffer;
	id<MTL4ArgumentTable>	argument_table;
} r_renderer;

id<MTLBuffer> r_make_triangle_vertex_buffer(void) {
	id <MTLBuffer> buffer = [r_renderer.device newBufferWithBytes:R_TRIANGLE_VERTICES
		length:sizeof(R_TRIANGLE_VERTICES)
		options:MTLResourceStorageModeShared];

	return buffer;
}

void r_make_command_allocators(void) {
	for (int i = 0; i < R_NUM_FRAMES_IN_FLIGHT; i++) {
		r_renderer.command_allocators[i] = [r_renderer.device newCommandAllocator];
	}
}

id<MTL4CommandAllocator> r_get_current_command_allocator(void) {
	return r_renderer.command_allocators[r_renderer.frame_number % R_NUM_FRAMES_IN_FLIGHT];
}

id<MTL4ArgumentTable> r_make_argument_table(void) {
	NSError* error = NULL;

	MTL4ArgumentTableDescriptor* descriptor = [[MTL4ArgumentTableDescriptor new] autorelease];
	descriptor.maxBufferBindCount = 2;

	id<MTL4ArgumentTable> argument_table = [r_renderer.device newArgumentTableWithDescriptor:descriptor
		error: &error];
	if (error != NULL) {
		printf("Failed to create an argument table. Aborting!");
		exit(-1);
	}

	return argument_table;
}

id<MTLResidencySet> r_make_residency_set(void) {
	NSError* error = NULL;

	MTLResidencySetDescriptor* descriptor = [[MTLResidencySetDescriptor new] autorelease];

	id<MTLResidencySet> residency_set = [r_renderer.device newResidencySetWithDescriptor:descriptor
		error: &error];
	if (error != NULL) {
		printf("Failed to create a residency set. Aborting!");
		exit(-1);
	}

	return residency_set;
}

id<MTLRenderPipelineState> r_make_renderpipeline(MTLPixelFormat pixel_format) {
	NSError* error = NULL;

	MTL4LibraryFunctionDescriptor* vertex_function_desc = [[MTL4LibraryFunctionDescriptor new] autorelease];
	vertex_function_desc.library = r_renderer.shader_library;
	vertex_function_desc.name = @"v_main";

	MTL4LibraryFunctionDescriptor* fragment_function_desc = [[MTL4LibraryFunctionDescriptor new] autorelease];
	fragment_function_desc.library = r_renderer.shader_library;
	fragment_function_desc.name = @"f_main";

	MTL4RenderPipelineDescriptor* descriptor = [[MTL4RenderPipelineDescriptor new] autorelease];
	descriptor.colorAttachments[0].pixelFormat = pixel_format;
	descriptor.vertexFunctionDescriptor = vertex_function_desc;
	descriptor.fragmentFunctionDescriptor = fragment_function_desc;

	// MTL4CompilerTaskOptions* options = [[MTL4CompilerTaskOptions new] autorelease];

	id<MTLRenderPipelineState> pipeline = [r_renderer.pipeline_compiler newRenderPipelineStateWithDescriptor:descriptor
		compilerTaskOptions:nil
		error:&error];
	if (pipeline == NULL || error != NULL) {
		printf("Could not compile the render pipeline. Aborting.");
		exit(-1);
	}

	return pipeline;
}

void r_init_metallayer_for_target_view(void) {
	CAMetalLayer* layer = [CAMetalLayer new];
	layer.device = r_renderer.device;
	layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
	layer.frame = r_renderer.target_view.bounds;

	r_renderer.target_view.wantsLayer = YES;
	r_renderer.target_view.layer = layer;

	r_renderer.target_view_layer = layer;
}

void r_init(NSView* view) {
	NSError* error = NULL;

	c_zero$(&r_renderer);

	@autoreleasepool {
		r_renderer.device = MTLCreateSystemDefaultDevice();
		if (![r_renderer.device supportsFamily:MTLGPUFamilyMetal4]) {
			printf("Sorry, this prototype needs Metal 4!");
			exit(-1);
		}

		r_renderer.target_view = view;
		r_init_metallayer_for_target_view();

		r_renderer.command_queue  = [r_renderer.device newMTL4CommandQueue];

		error = NULL;
		NSURL* shader_library_url = [NSURL fileURLWithPath:@"./res/shaders.metallib" isDirectory:NO];
		r_renderer.shader_library = [r_renderer.device newLibraryWithURL:shader_library_url error:&error];
		if (r_renderer.shader_library == NULL || error != NULL) {
			printf("%s\n", [error localizedDescription].UTF8String);
		}

		error = NULL;
		MTL4CompilerDescriptor* desc = [[MTL4CompilerDescriptor new] autorelease];
		r_renderer.pipeline_compiler = [r_renderer.device
			newCompilerWithDescriptor:desc
			error:&error];
		if (r_renderer.pipeline_compiler == NULL || error != NULL) {
			printf("%s\n", [error localizedDescription].UTF8String);
		}

		r_make_command_allocators();

		r_renderer.residency_set  = r_make_residency_set();
		r_renderer.triangle_vertex_buffer = r_make_triangle_vertex_buffer();
		r_renderer.render_pipeline = r_make_renderpipeline(r_renderer.target_view_layer.pixelFormat);
		r_renderer.argument_table = r_make_argument_table();

		[r_renderer.argument_table setAddress:r_renderer.triangle_vertex_buffer.gpuAddress atIndex:0];

		[r_renderer.residency_set addAllocation:r_renderer.triangle_vertex_buffer];
		[r_renderer.residency_set commit];

		r_renderer.frame_end_event = [r_renderer.device newSharedEvent];
		r_renderer.frame_end_event.signaledValue = 0;
	}
}

void r_update_viewport_size(NSSize viewport_size, id<MTL4RenderCommandEncoder> renderpass) {
	MTLViewport viewport;
	viewport.originX	= 0;
	viewport.originY	= 0;
	viewport.zfar		= 1.0;
	viewport.znear		= 0.0;
	viewport.width		= (double)viewport_size.width;
	viewport.height		= (double)viewport_size.height;

	[renderpass setViewport:viewport];
}

MTLRenderPassColorAttachmentDescriptor* r_get_colorattachment_for_target_view(void) {
	MTLRenderPassColorAttachmentDescriptor* descriptor = [[MTLRenderPassColorAttachmentDescriptor new] autorelease];

	descriptor.clearColor = (MTLClearColor){ 0.0, 0.0, 0.0, 0.0 };
	descriptor.texture = r_renderer.frame_drawable.texture;
	descriptor.loadAction = MTLLoadActionClear;
	descriptor.storeAction = MTLStoreActionStore;

	return descriptor;
}

MTL4RenderPassDescriptor* r_get_renderpass_for_target_view(void) {
	MTL4RenderPassDescriptor* descriptor = [[MTL4RenderPassDescriptor new] autorelease];

	MTLRenderPassColorAttachmentDescriptor* color_attachment = r_get_colorattachment_for_target_view();
	[descriptor.colorAttachments setObject:color_attachment atIndexedSubscript:0];

	return descriptor;
}

void r_render(void) {
	@autoreleasepool {
		r_renderer.frame_drawable = [r_renderer.target_view_layer nextDrawable];
		if (r_renderer.frame_drawable == nil) {
			return;
		}

		if (r_renderer.frame_number >= R_NUM_FRAMES_IN_FLIGHT) {
			BOOL before_timeout = [r_renderer.frame_end_event waitUntilSignaledValue:r_renderer.frame_number - R_NUM_FRAMES_IN_FLIGHT
				timeoutMS:100];

			if (!before_timeout) {
				printf("Warning: Frame %llu is late!", (unsigned long long)r_renderer.frame_number);
				return;
			}
		}

		id<MTL4CommandAllocator> command_allocator = r_get_current_command_allocator();
		[command_allocator reset];

		id<MTL4CommandBuffer> command = [[r_renderer.device newCommandBuffer] autorelease];
		[command beginCommandBufferWithAllocator:command_allocator];

		[r_renderer.command_queue addResidencySet:r_renderer.residency_set];

		MTL4RenderPassDescriptor* render_pass_descriptor = r_get_renderpass_for_target_view();
		
		id<MTL4RenderCommandEncoder> render_pass = [command
			renderCommandEncoderWithDescriptor:render_pass_descriptor];

		NSSize size = r_renderer.target_view.bounds.size;
		printf("%f %f\n", size.width, size.height);
		r_update_viewport_size(size, render_pass);
		[render_pass setRenderPipelineState:r_renderer.render_pipeline];
		[render_pass setArgumentTable:r_renderer.argument_table atStages:MTLRenderStageVertex];
		[render_pass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

		[render_pass endEncoding];
		[command endCommandBuffer];

		[r_renderer.command_queue waitForDrawable:r_renderer.frame_drawable];
		[r_renderer.command_queue commit:&command count:1];
		[r_renderer.command_queue signalDrawable:r_renderer.frame_drawable ];
		[r_renderer.frame_drawable present];

		r_renderer.frame_end_event.signaledValue = r_renderer.frame_number;
		[r_renderer.command_queue signalEvent:r_renderer.frame_end_event value:r_renderer.frame_number];
		r_renderer.frame_number += 1;
	}
}

void r_fini(void) {
	[r_renderer.triangle_vertex_buffer release];

	for (int i = 0; i < R_NUM_FRAMES_IN_FLIGHT; i++) {
		[r_renderer.command_allocators[i] release];
	}

	[r_renderer.frame_end_event release];

	[r_renderer.residency_set  release];
	[r_renderer.argument_table release];
	[r_renderer.shader_library release];
	[r_renderer.command_queue  release];
}


struct {
	RGFW_window* handle;
	NSView* content_view;
} w_window;

void w_init(void) {
	RGFW_window* window;
	window = RGFW_createWindow("Metal 4", 0, 0, 640, 480, RGFW_windowCenter);

	NSView* view = RGFW_window_getView_OSX(window);

	w_window.handle = window;
	w_window.content_view = view;

	r_init(w_window.content_view);
}

void w_loop(void) {
	while (RGFW_window_shouldClose(w_window.handle) == RGFW_FALSE) {
		RGFW_pollEvents();

		r_render();
	}
}

void w_fini(void) {
	RGFW_window_close(w_window.handle);
}

int main(void) {
	@autoreleasepool {
		w_init();
		w_loop();
		w_fini();
	}

	return 0;
}

