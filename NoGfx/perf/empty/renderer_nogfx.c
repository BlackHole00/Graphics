#include "renderer.h"
#include "main.h"

#ifdef NOGFX_RENDERER

#include <gpu/gpu.h>
#include <stdio.h>

struct {
	NSView*			view;
	GpuSurface		surface;

	GpuPipeline		renderPSO;

	GpuQueue		queue;

	GpuSemaphore		presentEvent;
	long			frameCount;

	Timer			waitTimer;
	Timer			encodeTimer;
	Timer			presentTimer;
} gRenderer;

GpuResult gResult;
#define CHECK_RES() assert(gResult == GPU_SUCCESS);

void initRenderer(void) {

	printf("Using NoGraphics rendererer.\n");

	createTimer(&gRenderer.waitTimer, "Wait", "./timings/wait.nogfx.csv", true);
	createTimer(&gRenderer.encodeTimer, "Encode", "./timings/encode.nogfx.csv", true);
	createTimer(&gRenderer.presentTimer, "Present", "./timings/present.nogfx.csv", true);


	GpuInitDesc initDesc = {
		GPU_METAL_4,
		false,
		false,
		NULL,
		0
	};
	gpuInit(&initDesc, &gResult);
	CHECK_RES();

	GpuDeviceInfo* deviceInfos;
	size_t devicesCount;
	gpuEnumerateDevices(&deviceInfos, &devicesCount, &gResult);
	CHECK_RES(); assert(devicesCount > 0);

	gpuSelectDevice(deviceInfos[0].identifier, &gResult);
	CHECK_RES();


	GpuFormat surfaceFormat = deviceInfos[0].capabilities.supportedSurfaceFormats[0];
	GpuSurfaceDesc surfaceDesc;
	// surfaceDesc.type = GPU_SURFACE_VSYNC;
	surfaceDesc.type = GPU_SURFACE_IMMEDIATE;
	surfaceDesc.format = surfaceFormat;
	surfaceDesc.framesInFlight = 3;
	surfaceDesc.size[0] = WINDOW_WIDTH;
	surfaceDesc.size[1] = WINDOW_HEIGHT;
	surfaceDesc.target.type = GPU_SURFACE_COCOA;
	surfaceDesc.target.cocoa.nsView = RGFW_window_getView_OSX(gState.window);
	gRenderer.surface = gpuCreateSurface(&surfaceDesc, &gResult); CHECK_RES();


	gRenderer.queue = gpuCreateQueue(&gResult); CHECK_RES();


	gRenderer.presentEvent = gpuCreateSemaphore(0, &gResult); CHECK_RES();
}

void finiRenderer(void) {
	destroyTimer(&gRenderer.waitTimer);
	destroyTimer(&gRenderer.encodeTimer);
	destroyTimer(&gRenderer.presentTimer);
}

void draw(void) {
	startTimer(&gRenderer.waitTimer);
	// if (gRenderer.frameCount > 3) {
	// 	gpuWaitSemaphore(gRenderer.presentEvent, gRenderer.frameCount, &gResult); CHECK_RES();
	// }
	if (gRenderer.frameCount > 1) {
		gpuWaitSemaphore(gRenderer.presentEvent, gRenderer.frameCount, &gResult); CHECK_RES();
	}
	stopTimer(&gRenderer.waitTimer);


	GpuTexture drawable = gpuAcquireNextDrawable(gRenderer.surface, &gResult);


	startTimer(&gRenderer.encodeTimer);
	GpuCommandBuffer commandBuffer = gpuStartCommandEncoding(gRenderer.queue, &gResult); CHECK_RES();

	GpuRenderTarget surfaceTarget = {};
	surfaceTarget.texture = drawable;
	surfaceTarget.loadOp = GPU_OP_CLEAR;
	surfaceTarget.storeOp = GPU_OP_STORE;
	surfaceTarget.clearColor[0] = 0.1;
	surfaceTarget.clearColor[1] = 0.1;
	surfaceTarget.clearColor[2] = 0.15;

	GpuRenderPassDesc renderPassDesc = {};
	renderPassDesc.colorTargets = &surfaceTarget;
	renderPassDesc.colorTargetCount = 1;

	gpuBeginRenderPass(commandBuffer, &renderPassDesc, &gResult); CHECK_RES();
	gpuEndRenderPass(commandBuffer, &gResult); CHECK_RES();
	gpuSubmitWithSignal(gRenderer.queue, &commandBuffer, 1, gRenderer.presentEvent, ++gRenderer.frameCount, &gResult); CHECK_RES();
	stopTimer(&gRenderer.encodeTimer);

	startTimer(&gRenderer.presentTimer);
	gpuPresent(gRenderer.queue, gRenderer.surface, &gResult); CHECK_RES();
	stopTimer(&gRenderer.presentTimer);
}

#endif
