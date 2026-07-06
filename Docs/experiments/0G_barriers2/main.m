#include <stdio.h>
#include <unistd.h>

#include <Metal/Metal.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

void mtl4BeginTracing(id<MTLDevice> device, const char* traceDestinationFile) {
	MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
	if (![captureManager supportsDestination:MTLCaptureDestinationGPUTraceDocument]) {
		printf(
			"WARN - Could not start a capture. Try starting the application with the following "
			"environment variables:\n"
			"\t- MTL_DEBUG_LAYER=1\n"
			"\t- MTL_CAPTURE_ENABLED=1\n"
		);
		return;
	}

	MTLCaptureDescriptor* captureDescriptor = [MTLCaptureDescriptor new];

	captureDescriptor.captureObject = device;
	captureDescriptor.destination = MTLCaptureDestinationGPUTraceDocument;
	captureDescriptor.outputURL = [NSURL fileURLWithPath: (NSString*)__CFStringMakeConstantString(traceDestinationFile)];

	if (access("/tmp/nogfx.gputrace", F_OK) == 0) {
		// NOTE: Removing a folder with the C standard library is difficult.
		system("rm -rf /tmp/nogfx.gputrace");
	}

	NSError* err = nil;
	[captureManager startCaptureWithDescriptor:captureDescriptor error:&err];
	if (err != nil) {
		printf("WARN - Could not start a capture:\n");
		printf("\t%s\n", [[err localizedFailureReason] UTF8String]);
		printf("\t%s\n", [[err localizedDescription] UTF8String]);
	}
}

void mtl4StopTracing(void) {
	MTLCaptureManager* captureManager = [MTLCaptureManager sharedCaptureManager];
	[captureManager stopCapture];
}


int main(void) {
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	mtl4BeginTracing(device, "trace.gputrace");

	MTL4CompilerDescriptor* compilerDesc = [MTL4CompilerDescriptor new];

	NSError *err = nil;
	id<MTL4Compiler> compiler = [device newCompilerWithDescriptor:compilerDesc error:&err];

	const char *nopSrc =
		"#include <metal_stdlib>\n"
		"using namespace metal;\n"
		"kernel void nop(uint2 gid [[thread_position_in_grid]]) { }\n";
	err = nil;
	NSString *src = [NSString stringWithUTF8String:nopSrc];

	id<MTLLibrary> library = [device newLibraryWithSource:src options:nil error:&err];
	MTL4LibraryFunctionDescriptor* function = [MTL4LibraryFunctionDescriptor new];
	function.library = library;
	function.name = @"nop";

	MTL4ComputePipelineDescriptor* pipelineDesc = [MTL4ComputePipelineDescriptor new];
	pipelineDesc.computeFunctionDescriptor = function;

	err = nil;
	id<MTLComputePipelineState> nopPipeline = [compiler newComputePipelineStateWithDescriptor:pipelineDesc compilerTaskOptions:nil error:&err];

	FILE* f = fopen("image.png", "rb");

	if (f == NULL) {
		printf("Could not open image.png\n");
		return -1;
	}

	int x, y, channels;
	uint8_t* data = stbi_load_from_file(f, &x, &y, &channels, 4);
	assert(channels == 4);

	fclose(f);

	id<MTLBuffer> uploadBuffer = [device newBufferWithLength:x * y * 4 options:MTLResourceStorageModeShared];
	id<MTLBuffer> downloadBuffer = [device newBufferWithLength:x * y * 4 options:MTLResourceStorageModeShared];

	memcpy(uploadBuffer.contents, data, x * y * 4);

	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
		width:x
		height:y
		mipmapped:false];
	textureDescriptor.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeTracked | MTLResourceCPUCacheModeDefaultCache;
	textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
	id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];

	id<MTL4CommandQueue> queue = [device newMTL4CommandQueue];
	id<MTL4CommandAllocator> allocator = [device newCommandAllocator];

	id<MTL4CommandBuffer> commandBuffer = [device newCommandBuffer];
	[commandBuffer beginCommandBufferWithAllocator:allocator];
	id<MTL4ComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

	MTLResidencySetDescriptor* residencySetDesc = [MTLResidencySetDescriptor new];
	err = nil;
	id<MTLResidencySet> residencySet = [device newResidencySetWithDescriptor:residencySetDesc error:&err];
	[residencySet addAllocation:texture];
	[residencySet commit];

	[queue addResidencySet:residencySet];

	[computeEncoder copyFromBuffer:uploadBuffer
		sourceOffset:0
		sourceBytesPerRow:x * 4
		sourceBytesPerImage:x* y * 4
		sourceSize:MTLSizeMake(x, y, 1)
		toTexture:texture
		destinationSlice:0
		destinationLevel:0
		destinationOrigin:MTLOriginMake(0, 0, 0)];

	[computeEncoder
		barrierAfterEncoderStages:MTLStageBlit
		beforeEncoderStages:MTLStageBlit
		visibilityOptions:MTL4VisibilityOptionDevice|MTL4VisibilityOptionResourceAlias];

	// [computeEncoder
	// 	barrierAfterEncoderStages:MTLStageBlit
	// 	beforeEncoderStages:MTLStageDispatch
	// 	visibilityOptions:MTL4VisibilityOptionResourceAlias];

	// MTL4ArgumentTableDescriptor* argumentTableDesc = [MTL4ArgumentTableDescriptor new];
	// argumentTableDesc.maxTextureBindCount = 1;
	// err = nil;
	// id<MTL4ArgumentTable> argumentTable = [device newArgumentTableWithDescriptor: argumentTableDesc error:&err];
	// [argumentTable setTexture:[texture gpuResourceID] atIndex:0];

	// [computeEncoder setArgumentTable:argumentTable];
	// [computeEncoder setComputePipelineState:nopPipeline];
	// [computeEncoder dispatchThreads:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
	// [computeEncoder
	// 	barrierAfterEncoderStages:MTLStageDispatch
	// 	beforeEncoderStages:MTLStageBlit
	// 	visibilityOptions:MTL4VisibilityOptionResourceAlias];

	// NOTE: Does not work
	// [computeEncoder
	// 	barrierAfterStages:MTLStageBlit
	// 	beforeQueueStages:MTLStageBlit
	// 	visibilityOptions:MTL4VisibilityOptionDevice];

	// NOTE: Does not work
	// [computeEncoder
	// 	barrierAfterStages:MTLStageBlit
	// 	beforeQueueStages:MTLStageBlit
	// 	visibilityOptions:MTL4VisibilityOptionNone];

	// NOTE: Does not work
	// id<MTLFence> fence = [device newFence];
	// [computeEncoder updateFence:fence afterEncoderStages:MTLStageBlit];
	// [computeEncoder waitForFence:fence beforeEncoderStages:MTLStageBlit];

	// id<MTLFence> fence = [device newFence];
	// [computeEncoder updateFence:fence afterEncoderStages:MTLStageBlit];
	// [computeEncoder endEncoding];

	// computeEncoder = [commandBuffer computeCommandEncoder];
	// [computeEncoder waitForFence:fence beforeEncoderStages:MTLStageBlit];

	// id<MTLEvent> event = [device newEvent];

	// [computeEncoder endEncoding];
	// [commandBuffer endCommandBuffer];
	// [queue commit:&commandBuffer count:1];
	// [queue signalEvent:event value:42];


	// commandBuffer = [device newCommandBuffer];
	// [commandBuffer beginCommandBufferWithAllocator:allocator];
	// computeEncoder = [commandBuffer computeCommandEncoder];

	[computeEncoder copyFromTexture:texture
		sourceSlice:0
		sourceLevel:0
		sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:MTLSizeMake(x, y, 1)
		toBuffer:downloadBuffer
		destinationOffset:0
		destinationBytesPerRow:x * 4
		destinationBytesPerImage:x * y * 4];

	[computeEncoder endEncoding];
	[commandBuffer endCommandBuffer];

	__block int finished = 0;

	MTL4CommitOptions* options = [MTL4CommitOptions new];
	[options addFeedbackHandler: ^(id<MTL4CommitFeedback>commitFeedback){
		finished = 1;
	}];
	// [queue waitForEvent:event value:42];
	[queue commit:&commandBuffer count:1 options:options];
	[queue release];

	while (finished == 0) {}

	usleep(1000000);

	for (int i = 0; i < x * y * 4; i++) {
		uint8_t value = ((uint8_t*)downloadBuffer.contents)[i];
		printf("%d ", value);
	}

	stbi_write_png("out.png", x, y, 4, [downloadBuffer contents], x * 4);
	stbi_image_free(data);
	
	mtl4StopTracing();
	return 0;
}

