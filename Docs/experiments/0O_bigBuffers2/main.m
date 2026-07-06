#include <stdio.h>
#include <unistd.h>
#include <stdatomic.h>

#include <Metal/Metal.h>

void mtl4BeginTracing(id<MTLDevice> device) {
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
	captureDescriptor.outputURL = [NSURL fileURLWithPath: @"trace.gputrace"];

	if (access("./trace.gputrace", F_OK) == 0) {
		// NOTE: Removing a folder with the C standard library is difficult.
		system("rm -rf ./trace.gputrace");
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
	// mtl4BeginTracing(device);

	const float data[8] = {
		0, 1, 2, 3, 4, 5, 6, 7
	};

	MTLResidencySetDescriptor* residencySetDescriptor = [MTLResidencySetDescriptor new];
	residencySetDescriptor.initialCapacity = 16;
	// id<MTLResidencySet> residencySet = [device newResidencySetWithDescriptor:(nonnull MTLResidencySetDescriptor *) error:(NSError * _Nullable * _Nullable)]
	id<MTLResidencySet> residencySet = [device newResidencySetWithDescriptor:residencySetDescriptor error:nil];

	// WORKS
	// id<MTLBuffer> uploadBuffer = [device newBufferWithLength:sizeof(data) options:MTLResourceStorageModeShared];
	// DOES NOT WORK
	id<MTLBuffer> uploadBuffer = [device newBufferWithLength:16 * 16 * 1024 options:MTLResourceStorageModeShared];

	id<MTLBuffer> downloadBuffer = [device newBufferWithLength:sizeof(data) options:MTLResourceStorageModeShared];
	id<MTLBuffer> gpuBuffer	= [device newBufferWithLength:sizeof(data) options:MTLResourceStorageModePrivate];

	[residencySet addAllocation:uploadBuffer];
	[residencySet addAllocation:downloadBuffer];
	[residencySet addAllocation:gpuBuffer];
	[residencySet commit];

	memcpy(uploadBuffer.contents, data, sizeof(data));

	id<MTL4CommandQueue> queue = [device newMTL4CommandQueue];
	id<MTL4CommandAllocator> allocator = [device newCommandAllocator];

	id<MTL4CommandBuffer> commandBuffer = [device newCommandBuffer];
	[commandBuffer beginCommandBufferWithAllocator:allocator];

	id<MTL4ComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

	[computeEncoder barrierAfterQueueStages:MTLStageAll beforeStages:MTLStageBlit visibilityOptions:MTL4VisibilityOptionDevice | MTL4VisibilityOptionResourceAlias];
	[computeEncoder copyFromBuffer:uploadBuffer sourceOffset:0 toBuffer:gpuBuffer destinationOffset:0 size:8*sizeof(float)];
	[computeEncoder endEncoding];

	computeEncoder = [commandBuffer computeCommandEncoder];
	[computeEncoder barrierAfterQueueStages:MTLStageAll beforeStages:MTLStageBlit visibilityOptions:MTL4VisibilityOptionDevice];
	[computeEncoder copyFromBuffer:gpuBuffer sourceOffset:0 toBuffer:downloadBuffer destinationOffset:0 size:8*sizeof(float)];

	[computeEncoder endEncoding];
	[commandBuffer endCommandBuffer];
	
	id<MTLSharedEvent> sharedEvent = [device newSharedEvent];
	[queue addResidencySet:residencySet];
	[queue commit:&commandBuffer count:1];
	[queue signalEvent:sharedEvent value:1];

	[queue release];

	[sharedEvent waitUntilSignaledValue:1 timeoutMS:-1];

	int count = sizeof(data) / sizeof(*data);
	for (int i = 0; i < count; i++) {
		float value = ((float*)downloadBuffer.contents)[i];
		printf("%f ", value);
	}
	printf("\n");

	// mtl4StopTracing();
	return 0;
}

