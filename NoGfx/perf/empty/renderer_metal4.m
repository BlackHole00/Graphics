#include "renderer.h"
#include "main.h"

#ifdef METAL4_RENDERER

#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>
#include <stdio.h>

struct {
	NSView*			view;
	CAMetalLayer*		layer;

	id<MTLDevice>		device;
	id<MTL4CommandAllocator>	commandAllocator;


	id<MTL4CommandQueue>	queue;

	id<MTLSharedEvent>	presentEvent;
	long			frameCount;

	Timer			waitTimer;
	Timer			encodeTimer;
	Timer			presentTimer;

} gRenderer;

void initRenderer(void) { @autoreleasepool {

	printf("Using Metal 4 rendererer.\n");

	createTimer(&gRenderer.waitTimer, "Wait", "./timings/wait.metal4.csv", true);
	createTimer(&gRenderer.encodeTimer, "Encode", "./timings/encode.metal4.csv", true);
	createTimer(&gRenderer.presentTimer, "Present", "./timings/present.metal4.csv", true);


	gRenderer.device = MTLCreateSystemDefaultDevice();


	gRenderer.view = (NSView*)RGFW_window_getView_OSX(gState.window);

	gRenderer.layer = [CAMetalLayer new];
	gRenderer.layer.device = gRenderer.device;
	gRenderer.layer.frame = gRenderer.view.frame;
	gRenderer.layer.maximumDrawableCount = 3;
	gRenderer.layer.displaySyncEnabled = NO;
	gRenderer.view.wantsLayer = YES;
	gRenderer.view.layer = gRenderer.layer;


	gRenderer.commandAllocator = [gRenderer.device newCommandAllocator];


	gRenderer.queue = [gRenderer.device newMTL4CommandQueue];
	[gRenderer.queue addResidencySet:gRenderer.layer.residencySet];


	gRenderer.presentEvent = [gRenderer.device newSharedEvent];
	gRenderer.presentEvent.signaledValue = 0;
}}

void finiRenderer(void) {
	destroyTimer(&gRenderer.waitTimer);
	destroyTimer(&gRenderer.encodeTimer);
	destroyTimer(&gRenderer.presentTimer);
}

void draw(void) { @autoreleasepool {
	startTimer(&gRenderer.waitTimer);
	if (gRenderer.frameCount > 3) {
		[gRenderer.presentEvent waitUntilSignaledValue:gRenderer.frameCount timeoutMS:-1];
	}
	// if (gRenderer.frameCount > 1) {
	// 	[gRenderer.presentEvent waitUntilSignaledValue:gRenderer.frameCount timeoutMS:-1];
	// }
	stopTimer(&gRenderer.waitTimer);

	id<CAMetalDrawable> drawable = [gRenderer.layer nextDrawable];

	MTL4RenderPassDescriptor* renderPassDesc = [[MTL4RenderPassDescriptor new] autorelease];
	renderPassDesc.colorAttachments[0].texture = drawable.texture;
	renderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.14, 0.0);
	renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
	renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

	startTimer(&gRenderer.encodeTimer);

	id<MTL4CommandBuffer> commandBuffer = [[gRenderer.device newCommandBuffer] autorelease];
	[commandBuffer beginCommandBufferWithAllocator:gRenderer.commandAllocator];

	id<MTL4RenderCommandEncoder> renderpass = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
	[renderpass endEncoding];

	[commandBuffer endCommandBuffer];
	stopTimer(&gRenderer.encodeTimer);

	[gRenderer.queue waitForDrawable:drawable];
	[gRenderer.queue commit:&commandBuffer count: 1];
	[gRenderer.queue signalEvent:gRenderer.presentEvent value:++gRenderer.frameCount];

	startTimer(&gRenderer.presentTimer);
	[gRenderer.queue signalDrawable:drawable];
	[drawable present];
	stopTimer(&gRenderer.presentTimer);
} }

#endif
