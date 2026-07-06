#include "renderer.h"
#include "main.h"

#ifdef NOGFX_RENDERER

#include <gpu/gpu.h>
#include <stdio.h>

typedef struct GpuAllocation {
	uint8_t*	cpu;
	uint8_t*	gpu;
} GpuAllocation;

typedef struct GpuBumpAllocator {
	uint8_t*	cpu;
	uint8_t*	gpu;
	uint32_t	size;
	uint32_t	offset;
} GpuBumpAllocator;

typedef struct GpuArena {
	uint8_t*	cpu;
	uint8_t*	gpu;
	size_t		size;
	size_t		offset;
} GpuArena;

typedef struct DrawVertexArgs {
	void*		vertices;
	Position2	position;
} DrawVertexArgs;

typedef struct DrawFragmentArgs {
	float		color[3];
} DrawFragmentArgs;

struct {
	NSView*			view;
	GpuSurface		surface;

	GpuBumpAllocator	tempDataAllocator;
	GpuArena		privateDataAllocator;

	GpuAllocation		triangleVertices;
	GpuAllocation		triangleIndices;
	GpuPipeline		renderPSO;

	GpuQueue		queue;

	GpuSemaphore		setupPrepComplete;
	GpuSemaphore		frameDoneEvent;
	long			frameCount;

	Timer			waitTimer;
	Timer			encodeTimer;
	Timer			presentTimer;
} gRenderer;

GpuResult gResult;
#define CHECK_RES() assert(gResult == GPU_SUCCESS);

uint8_t* readEntireFile(const char* file, size_t* fileLength) {
	FILE* handle = fopen(file, "rb");
	if (handle == NULL) {
		return NULL;
	}

	fseek(handle, 0L, SEEK_END);
	*fileLength = ftell(handle);
	fseek(handle, 0L, SEEK_SET);	

	uint8_t* buffer = (uint8_t*)calloc(1, *fileLength);
	if (buffer == NULL) {
		return NULL;
	}

	fread(buffer, sizeof(uint8_t), *fileLength, handle);
	fclose(handle);

	return buffer;
}

void gpuCreateBump(GpuBumpAllocator* allocator, size_t size, GpuMemory memory) {
	allocator->cpu = (uint8_t*)gpuMalloc(size, 16, memory, NULL);
	allocator->gpu = (uint8_t*)gpuHostToDevicePointer(allocator->cpu, NULL);
	allocator->offset = 0;
	allocator->size = size;
}

GpuAllocation gpuBumpAlloc(GpuBumpAllocator* allocator, size_t bytes) {
	if (allocator->offset + bytes >= allocator->size) {
		allocator->offset = 0;
	}

	GpuAllocation alloc;
	alloc.cpu = allocator->cpu ? allocator->cpu + allocator->offset : 0;
	alloc.gpu = allocator->gpu + allocator->offset;

	allocator->offset += bytes;

	return alloc;
}

GpuAllocation gpuBumpAllocAligned(GpuBumpAllocator* allocator, size_t bytes, size_t align) {
	if (align == 0) {
		align = 1;
	}

	if (allocator->offset + bytes >= allocator->size) {
		allocator->offset = 0;
	}

	size_t mask = align - 1;
	size_t alignedOffset = (allocator->offset + mask) & ~mask;

	GpuAllocation alloc;
	alloc.cpu = allocator->cpu ? allocator->cpu + alignedOffset : 0;
	alloc.gpu = allocator->gpu + alignedOffset;

	allocator->offset += bytes;

	return alloc;
}

void gpuCreateArena(GpuArena* arena, size_t size, GpuMemory memory) {
	if (memory == GPU_MEMORY_GPU) {
		arena->cpu = 0;
		arena->gpu = (uint8_t*)gpuMalloc(size, 16, GPU_MEMORY_GPU, NULL);
	} else {
		arena->cpu = (uint8_t*)gpuMalloc(size, 16, memory, NULL);
		arena->gpu = (uint8_t*)gpuHostToDevicePointer(arena->cpu, NULL);
	}

	arena->size = size;
	arena->offset = 0;
}

GpuAllocation gpuArenaAlloc(GpuArena* arena, size_t size) {
	if (arena->offset + size > arena->size) {
		GpuAllocation allocation = {};
		return allocation;
	}

	size_t oldOffset = arena->offset;
	arena->offset += size;

	GpuAllocation allocation = {
		arena->cpu ? arena->cpu + oldOffset : NULL,
		arena->gpu + oldOffset
	};
	return allocation;
}

GpuAllocation gpuArenaAllocAligned(GpuArena* arena, size_t size, size_t align) {
	if (align == 0) {
		align = 1;
	}

	size_t mask = align - 1;
	size_t alignedOffset = (arena->offset + mask) & ~mask;

	if (alignedOffset + size > arena->size) {
		GpuAllocation allocation = {};
		return allocation;
	}

	arena->offset = alignedOffset + size;

	GpuAllocation allocation = {
		arena->cpu ? arena->cpu + alignedOffset : NULL,
		arena->gpu + alignedOffset
	};
	return allocation;
}

void initRenderer(void) {

	printf("Using NoGraphics rendererer.\n");

	createTimer(&gRenderer.waitTimer, "Wait", "./timings/wait.nogfx.csv", false);
	createTimer(&gRenderer.encodeTimer, "Encode", "./timings/encode.nogfx.csv", false);
	createTimer(&gRenderer.presentTimer, "Present", "./timings/present.nogfx.csv", false);


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


	size_t vertexIrSize, fragmentIrSize;
	uint8_t* vertexIr = readEntireFile("triangle.vertex.metallib", &vertexIrSize);
	uint8_t* fragmentIr = readEntireFile("triangle.fragment.metallib", &fragmentIrSize);

	GpuColorTarget surfaceColorTarget = {
		surfaceFormat,
		0xF,
	};
	GpuRasterDesc rasterDesc = {
		GPU_TOPOLOGY_TRIANGLE_LIST,
		GPU_CULL_NONE,
		false,
		false,
		1,
		GPU_FORMAT_NONE,
		GPU_FORMAT_NONE,
		&surfaceColorTarget,
		1,
		NULL
	};

	float constants[2] = { WINDOW_WIDTH, WINDOW_HEIGHT };
	gRenderer.renderPSO = gpuCreateRenderPipeline(
		vertexIr, vertexIrSize,
		fragmentIr, fragmentIrSize,
		&constants, sizeof(float) * 2,
		NULL, 0,
		&rasterDesc, &gResult
	); CHECK_RES();



	gpuCreateArena(&gRenderer.privateDataAllocator, 1024 * 16, GPU_MEMORY_GPU);
	gpuCreateBump(&gRenderer.tempDataAllocator, 1024 * 1024 * 16, GPU_MEMORY_DEFAULT);

	gRenderer.triangleVertices = gpuArenaAllocAligned(&gRenderer.privateDataAllocator, sizeof(TRIANGLE_VERTICES), 16);
	gRenderer.triangleIndices = gpuArenaAllocAligned(&gRenderer.privateDataAllocator, sizeof(TRIANGLE_INDICES), 16);


	gRenderer.queue = gpuCreateQueue(&gResult); CHECK_RES();


	gRenderer.frameDoneEvent = gpuCreateSemaphore(0, &gResult); CHECK_RES();
	gRenderer.setupPrepComplete = gpuCreateSemaphore(0, &gResult); CHECK_RES();


	GpuCommandBuffer uploadCb = gpuStartCommandEncoding(gRenderer.queue, &gResult); CHECK_RES();

	GpuAllocation verticesUpload = gpuBumpAlloc(&gRenderer.tempDataAllocator, sizeof(TRIANGLE_VERTICES));
	memcpy(verticesUpload.cpu, &TRIANGLE_VERTICES[0], sizeof(TRIANGLE_VERTICES));
	gpuMemCpy(uploadCb, gRenderer.triangleVertices.gpu, verticesUpload.gpu, sizeof(TRIANGLE_VERTICES), &gResult); CHECK_RES();

	GpuAllocation indicesUpload = gpuBumpAlloc(&gRenderer.tempDataAllocator, sizeof(TRIANGLE_INDICES));
	memcpy(indicesUpload.cpu, &TRIANGLE_INDICES[0], sizeof(TRIANGLE_INDICES));
	gpuMemCpy(uploadCb, gRenderer.triangleIndices.gpu, indicesUpload.gpu, sizeof(TRIANGLE_INDICES), &gResult); CHECK_RES();

	gpuSubmitWithSignal(gRenderer.queue, &uploadCb, 1, gRenderer.setupPrepComplete, 1, &gResult); CHECK_RES();
	gpuWaitSemaphore(gRenderer.setupPrepComplete, 1, &gResult); CHECK_RES();
}

void finiRenderer(void) {
	destroyTimer(&gRenderer.waitTimer);
	destroyTimer(&gRenderer.encodeTimer);
	destroyTimer(&gRenderer.presentTimer);
}

void draw(void) {
	startTimer(&gRenderer.waitTimer);
	if (gRenderer.frameCount > 3) {
		gpuWaitSemaphore(gRenderer.frameDoneEvent, gRenderer.frameCount, &gResult); CHECK_RES();
	}
	stopTimerF(&gRenderer.waitTimer, ",%lld", gState.activeTriangles);


	GpuTexture drawable = gpuAcquireNextDrawable(gRenderer.surface, &gResult);


	startTimer(&gRenderer.encodeTimer);
	GpuCommandBuffer commandBuffer = gpuStartCommandEncoding(gRenderer.queue, &gResult); CHECK_RES();

	GpuRenderTarget surfaceTarget = {};
	surfaceTarget.texture = drawable;
	surfaceTarget.loadOp = GPU_OP_CLEAR;
	surfaceTarget.storeOp = GPU_OP_STORE;
	surfaceTarget.clearColor[0] = 0.2;
	surfaceTarget.clearColor[1] = 0.1;
	surfaceTarget.clearColor[2] = 1.5;

	GpuRenderPassDesc renderPassDesc = {};
	renderPassDesc.colorTargets = &surfaceTarget;
	renderPassDesc.colorTargetCount = 1;

	gpuBeginRenderPass(commandBuffer, &renderPassDesc, &gResult); CHECK_RES();

	gpuSetPipeline(commandBuffer, gRenderer.renderPSO, &gResult); CHECK_RES();
	for (int i = 0; i < gState.activeTriangles; i++) {
		GpuAllocation vertexArgs = gpuBumpAlloc(&gRenderer.tempDataAllocator, sizeof(DrawVertexArgs));

		((DrawVertexArgs*)vertexArgs.cpu)->vertices = gRenderer.triangleVertices.gpu;
		((DrawVertexArgs*)vertexArgs.cpu)->position = gState.triangles[i];

		gpuDrawIndexedInstanced(commandBuffer, vertexArgs.gpu, NULL, gRenderer.triangleIndices.gpu, 3, 1, &gResult); CHECK_RES();
	}

	gpuEndRenderPass(commandBuffer, &gResult); CHECK_RES();
	gpuSubmitWithSignal(gRenderer.queue, &commandBuffer, 1, gRenderer.frameDoneEvent, ++gRenderer.frameCount, &gResult); CHECK_RES();
	stopTimerF(&gRenderer.encodeTimer, ",%lld", gState.activeTriangles);

	startTimer(&gRenderer.presentTimer);
	gpuPresent(gRenderer.queue, gRenderer.surface, &gResult); CHECK_RES();
	stopTimerF(&gRenderer.presentTimer, ",%lld", gState.activeTriangles);
}

#endif
