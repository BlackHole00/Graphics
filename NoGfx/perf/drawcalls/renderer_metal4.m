#include "renderer.h"
#include "main.h"

#ifdef METAL4_RENDERER

#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>

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

typedef struct {
	id<MTLBuffer>		buffer;
	size_t			offset;
	void*			cpu;
	uintptr_t		gpu;
} MTLAllocation;

typedef struct {
	id<MTLHeap>		heap;
	id<MTLBuffer>		buffer;
	MTLResourceOptions	resourceOptions;
	size_t			size;
	size_t			offset;
	uintptr_t		cpu;
	uintptr_t		gpu;
} MTLBumpAllocator;

struct {
	NSView*			view;
	CAMetalLayer*		layer;

	id<MTLDevice>		device;
	id<MTL4Compiler>	compiler;
	id<MTL4CommandAllocator>	commandAllocator;

	id<MTL4ArgumentTable>	argumentTable;

	id<MTLResidencySet>	residencySet;
	MTLBumpAllocator	bumpAllocator;
	id<MTLHeap>		privateHeap;

	id<MTLBuffer>		vertices;

	id<MTLRenderPipelineState>	renderPSO;

	id<MTL4CommandQueue>	queue;

	id<MTLSharedEvent>	presentEvent;
	long			frameCount;

	Timer			waitTimer;
	Timer			encodeTimer;
	Timer			presentTimer;
} gRenderer;


void MTLCreateBump(MTLBumpAllocator* allocator, size_t size, MTLResourceOptions options) {
	MTLHeapDescriptor* heapDesc = [[MTLHeapDescriptor new] autorelease];
	heapDesc.resourceOptions = options;
	heapDesc.type = MTLHeapTypePlacement;
	heapDesc.size = size;

	allocator->heap = [gRenderer.device newHeapWithDescriptor:heapDesc];
	allocator->offset = 0;
	allocator->size = size;

	allocator->buffer	= [allocator->heap newBufferWithLength:16 options:options offset:0];
	allocator->gpu		= [allocator->buffer gpuAddress];
	allocator->cpu	= (uintptr_t)[allocator->buffer contents];
}

MTLAllocation MTLBumpAlloc(MTLBumpAllocator* allocator, size_t bytes) {
	if (allocator->offset + bytes >= allocator->size) {
		allocator->offset = 0;
	}

	MTLAllocation alloc;
	alloc.cpu = allocator->cpu ? (void*)(allocator->cpu + allocator->offset) : 0;
	alloc.gpu = allocator->gpu + allocator->offset;
	alloc.buffer = allocator->buffer;
	alloc.offset = allocator->offset;

	allocator->offset += bytes;

	return alloc;
}

MTLAllocation MTLBumpAllocAligned(MTLBumpAllocator* allocator, size_t bytes, size_t align) {
	if (align == 0) {
		align = 1;
	}

	if (allocator->offset + bytes >= allocator->size) {
		allocator->offset = 0;
	}

	size_t mask = align - 1;
	size_t alignedOffset = (allocator->offset + mask) & ~mask;

	MTLAllocation alloc;
	alloc.cpu = allocator->cpu ? (void*)(allocator->cpu + alignedOffset) : 0;
	alloc.gpu = allocator->gpu + alignedOffset;
	alloc.buffer = allocator->buffer;
	alloc.offset = alignedOffset;

	allocator->offset += bytes;

	return alloc;
}

void initRenderer(void) { @autoreleasepool {
	printf("Using Metal 4 rendererer.\n");

	createTimer(&gRenderer.waitTimer, "Wait", "./timings/wait.metal4.csv", false);
	createTimer(&gRenderer.encodeTimer, "Encode", "./timings/encode.metal4.csv", false);
	createTimer(&gRenderer.presentTimer, "Present", "./timings/present.metal4.csv", false);


	gRenderer.device = MTLCreateSystemDefaultDevice();


	gRenderer.view = (NSView*)RGFW_window_getView_OSX(gState.window);

	gRenderer.layer = [CAMetalLayer new];
	gRenderer.layer.device = gRenderer.device;
	gRenderer.layer.frame = gRenderer.view.frame;
	gRenderer.layer.maximumDrawableCount = 3;
	gRenderer.layer.displaySyncEnabled = YES;
	gRenderer.view.wantsLayer = YES;
	gRenderer.view.layer = gRenderer.layer;


	MTLHeapDescriptor* heapDesc = [[MTLHeapDescriptor new] autorelease];
	heapDesc.size = 1024 * 16;
	heapDesc.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeTracked;

	gRenderer.privateHeap = [gRenderer.device newHeapWithDescriptor:heapDesc];

	MTLCreateBump(&gRenderer.bumpAllocator, 1024 * 1024 * 16, MTLResourceStorageModeShared | MTLResourceHazardTrackingModeTracked);

	gRenderer.vertices = [gRenderer.privateHeap newBufferWithLength:sizeof(Vertex) * 3 options:MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeTracked];

	gRenderer.commandAllocator = [gRenderer.device newCommandAllocator];


	MTL4CompilerDescriptor* compilerDesc = [[MTL4CompilerDescriptor new] autorelease];
	gRenderer.compiler = [gRenderer.device newCompilerWithDescriptor:compilerDesc error:nil];

	size_t irSize;
	uint8_t* ir = readEntireFile("./triangle.metallib", &irSize);

	dispatch_data_t data = dispatch_data_create(ir, irSize, dispatch_get_main_queue(), NULL);
	id<MTLLibrary> library = [gRenderer.device newLibraryWithData:data error:nil];
	assert(library != NULL);

	MTL4LibraryFunctionDescriptor* vertexBaseFunction = [[MTL4LibraryFunctionDescriptor new] autorelease];
	vertexBaseFunction.name = @"vertexMain";
	vertexBaseFunction.library = library;
	
	float windowWidth = WINDOW_WIDTH, windowHeight = WINDOW_HEIGHT;
	MTLFunctionConstantValues* vertexConstants = [[MTLFunctionConstantValues new] autorelease];
	[vertexConstants setConstantValue:&windowWidth type:MTLDataTypeFloat atIndex:0];
	[vertexConstants setConstantValue:&windowHeight type:MTLDataTypeFloat atIndex:1];

	MTL4SpecializedFunctionDescriptor* vertexFunction = [[MTL4SpecializedFunctionDescriptor new] autorelease];
	vertexFunction.constantValues = vertexConstants;
	vertexFunction.functionDescriptor = vertexBaseFunction;

	MTL4LibraryFunctionDescriptor* fragmentFunction = [[MTL4LibraryFunctionDescriptor new] autorelease];
	fragmentFunction.name = @"fragmentMain";
	fragmentFunction.library = library;


	MTL4RenderPipelineDescriptor* pipelineDesc = [[MTL4RenderPipelineDescriptor new] autorelease];
	pipelineDesc.vertexFunctionDescriptor = vertexFunction;
	pipelineDesc.fragmentFunctionDescriptor = fragmentFunction;
	pipelineDesc.colorAttachments[0].pixelFormat = gRenderer.layer.pixelFormat;

	gRenderer.renderPSO = [gRenderer.compiler newRenderPipelineStateWithDescriptor:pipelineDesc compilerTaskOptions:nil error:nil];


	MTLResidencySetDescriptor* residencySetDesc = [[MTLResidencySetDescriptor new] autorelease];
	residencySetDesc.initialCapacity = 1;

	gRenderer.residencySet = [gRenderer.device newResidencySetWithDescriptor:residencySetDesc error:nil];
	[gRenderer.residencySet addAllocation:gRenderer.privateHeap];
	[gRenderer.residencySet addAllocation:gRenderer.bumpAllocator.heap];
	[gRenderer.residencySet commit];


	MTL4ArgumentTableDescriptor* argumentTableDesc = [[MTL4ArgumentTableDescriptor new] autorelease];
	argumentTableDesc.maxBufferBindCount = 2;

	gRenderer.argumentTable = [gRenderer.device newArgumentTableWithDescriptor:argumentTableDesc error:nil];


	gRenderer.queue = [gRenderer.device newMTL4CommandQueue];
	[gRenderer.queue addResidencySet:gRenderer.residencySet];
	[gRenderer.queue addResidencySet:gRenderer.layer.residencySet];


	gRenderer.presentEvent = [gRenderer.device newSharedEvent];
	gRenderer.presentEvent.signaledValue = 0;


	id<MTLSharedEvent> setupDoneEvent = [[gRenderer.device newSharedEvent] autorelease];


	MTLAllocation vertexUpload = MTLBumpAlloc(&gRenderer.bumpAllocator, sizeof(TRIANGLE_VERTICES));
	memcpy(vertexUpload.cpu, &TRIANGLE_VERTICES[0], sizeof(TRIANGLE_VERTICES));

	id<MTL4CommandBuffer> uploadCb = [[gRenderer.device newCommandBuffer] autorelease];
	[uploadCb beginCommandBufferWithAllocator:gRenderer.commandAllocator];

	id<MTL4ComputeCommandEncoder> compute = [uploadCb computeCommandEncoder];
	[compute copyFromBuffer:vertexUpload.buffer sourceOffset:vertexUpload.offset toBuffer:gRenderer.vertices destinationOffset:0 size:sizeof(TRIANGLE_VERTICES)];
	[compute endEncoding];

	[uploadCb endCommandBuffer];

	[gRenderer.queue commit:&uploadCb count:1];
	[gRenderer.queue signalEvent:setupDoneEvent value:1];
	[setupDoneEvent waitUntilSignaledValue:1 timeoutMS:-1];
}}

void finiRenderer() {}

void draw(void) { @autoreleasepool {
	startTimer(&gRenderer.waitTimer);
	if (gRenderer.frameCount > 3) {
		[gRenderer.presentEvent waitUntilSignaledValue:gRenderer.frameCount timeoutMS:-1];
	}
	stopTimerF(&gRenderer.waitTimer, ",%lld", gState.activeTriangles);


	id<CAMetalDrawable> drawable = [gRenderer.layer nextDrawable];

	MTL4RenderPassDescriptor* renderPassDesc = [[MTL4RenderPassDescriptor new] autorelease];
	renderPassDesc.colorAttachments[0].texture = drawable.texture;
	renderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.2, 0.1, 1.5, 0.0);
	renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
	renderPassDesc.colorAttachments[0].storeAction = MTLStoreActionStore;


	startTimer(&gRenderer.encodeTimer);

	[gRenderer.commandAllocator reset];
	id<MTL4CommandBuffer> commandBuffer = [[gRenderer.device newCommandBuffer] autorelease];
	[commandBuffer beginCommandBufferWithAllocator:gRenderer.commandAllocator];

	id<MTL4RenderCommandEncoder> renderpass = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];

	[renderpass setRenderPipelineState:gRenderer.renderPSO];
	for (int i = 0; i < gState.activeTriangles; i++) {
		MTLAllocation drawArgs = MTLBumpAlloc(&gRenderer.bumpAllocator, sizeof(Position2));
		*(Position2*)drawArgs.cpu = gState.triangles[i];

		[gRenderer.argumentTable setAddress:drawArgs.gpu atIndex:0];
		[gRenderer.argumentTable setAddress:[gRenderer.vertices gpuAddress] atIndex:1];

		[renderpass setArgumentTable:gRenderer.argumentTable atStages:MTLRenderStageVertex];
		[renderpass drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3 instanceCount:1];
	}

	[renderpass endEncoding];

	[commandBuffer endCommandBuffer];

	[gRenderer.queue waitForDrawable:drawable];
	[gRenderer.queue commit:&commandBuffer count: 1];
	[gRenderer.queue signalEvent:gRenderer.presentEvent value:++gRenderer.frameCount];
	stopTimerF(&gRenderer.encodeTimer, ",%lld", gState.activeTriangles);

	startTimer(&gRenderer.presentTimer);
	[gRenderer.queue signalDrawable:drawable];
	[drawable present];
	stopTimerF(&gRenderer.presentTimer, ",%lld", gState.activeTriangles);
} }

#endif
