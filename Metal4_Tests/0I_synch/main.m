#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

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

#include "wait.metal"
#include "signal.metal"

enum WaitOp {
	WAIT_OP_NEVER,
	WAIT_OP_LESS,
	WAIT_OP_EQUAL,
	WAIT_OP_LESS_EQUAL,
	WAIT_OP_GREATER,
	WAIT_OP_NOT_EQUAL,
	WAIT_OP_GREATER_EQUAL,
	WAIT_OP_ALWAYS,
	WAIT_OP_COUNT,
};

enum SignalOp {
	SIGNAL_OP_ATOMIC_SET,
	SIGNAL_OP_ATOMIC_MAX,
	SIGNAL_OP_ATOMIC_OR,
	SIGNAL_OP_COUNT,
};

id<MTLComputePipelineState> waitPipelines	[WAIT_OP_COUNT];
id<MTLComputePipelineState> signalPipelines	[SIGNAL_OP_COUNT];

void prepareWaitPipelines(id<MTLDevice> device, id<MTL4Compiler> compiler) {
	NSError* error = nil;
	id<MTLLibrary> library = [device newLibraryWithSource:(NSString*)__CFStringMakeConstantString(waitKernelSource)
		options:nil
		error:&error];
	if (error != nil) {
		printf(
			"%s\n%s\n",
			[[error localizedFailureReason] UTF8String],
			[[error localizedDescription] UTF8String]
		);
	}

	MTL4LibraryFunctionDescriptor* unspecializedWait = [MTL4LibraryFunctionDescriptor new];
	unspecializedWait.library = library;
	unspecializedWait.name = @"waitFor";

	for (size_t i = 0; i < WAIT_OP_COUNT; i++) {
		MTLFunctionConstantValues* functionConstants = [MTLFunctionConstantValues new];
		[functionConstants setConstantValue:&i type:MTLDataTypeUInt atIndex:0];

		MTL4SpecializedFunctionDescriptor* func = [MTL4SpecializedFunctionDescriptor new];
		func.functionDescriptor = unspecializedWait;
		func.constantValues = functionConstants;

		MTL4ComputePipelineDescriptor* pipelineDesc = [MTL4ComputePipelineDescriptor new];
		pipelineDesc.computeFunctionDescriptor = func;
		pipelineDesc.maxTotalThreadsPerThreadgroup = 1;

		error = nil;
		id<MTLComputePipelineState> pipeline = [compiler
			newComputePipelineStateWithDescriptor:pipelineDesc
			compilerTaskOptions:nil
			error:&error];
		if (error != nil) {
			printf(
				"%s\n%s\n",
				[[error localizedFailureReason] UTF8String],
				[[error localizedDescription] UTF8String]
			);
		}

		waitPipelines[i] = pipeline;
	}
	
}

void prepareSignalPipelines(id<MTLDevice> device, id<MTL4Compiler> compiler) {
	NSError* error = nil;
	id<MTLLibrary> library = [device newLibraryWithSource:(NSString*)__CFStringMakeConstantString(signalKernelSource)
		options:nil
		error:&error];
	if (error != nil) {
		printf(
			"%s\n%s\n",
			[[error localizedFailureReason] UTF8String],
			[[error localizedDescription] UTF8String]
		);
	}

	MTL4LibraryFunctionDescriptor* unspecializedWait = [MTL4LibraryFunctionDescriptor new];
	unspecializedWait.library = library;
	unspecializedWait.name = @"signalValue";

	for (size_t i = 0; i < SIGNAL_OP_COUNT; i++) {
		MTLFunctionConstantValues* functionConstants = [MTLFunctionConstantValues new];
		[functionConstants setConstantValue:&i type:MTLDataTypeUInt atIndex:0];

		MTL4SpecializedFunctionDescriptor* func = [MTL4SpecializedFunctionDescriptor new];
		func.functionDescriptor = unspecializedWait;
		func.constantValues = functionConstants;

		MTL4ComputePipelineDescriptor* pipelineDesc = [MTL4ComputePipelineDescriptor new];
		pipelineDesc.computeFunctionDescriptor = func;
		pipelineDesc.maxTotalThreadsPerThreadgroup = 1;

		error = nil;
		id<MTLComputePipelineState> pipeline = [compiler
			newComputePipelineStateWithDescriptor:pipelineDesc
			compilerTaskOptions:nil
			error:&error];
		if (error != nil) {
			printf(
				"%s\n%s\n",
				[[error localizedFailureReason] UTF8String],
				[[error localizedDescription] UTF8String]
			);
		}

		signalPipelines[i] = pipeline;
	}
}

struct WaitTask {
	uintptr_t	signalSeq;
	uint32_t	initialSignalSeq;
	uintptr_t	signalNumber;
	uint32_t	waitSignalNumber;
};

struct SignalTask {
	uintptr_t	signalSeq;
	uintptr_t	signalNumber;
	uint		value;
};

uint32_t	currentSeq;
id<MTLBuffer>	seqBuffer;
id<MTLBuffer>	signalBuffer;

void gpuWait(id<MTLDevice> device, id<MTL4ComputeCommandEncoder> computeEncoder, uint32_t value, enum WaitOp waitOp) {
	struct WaitTask task;
	task.signalSeq		= seqBuffer.gpuAddress;
	task.initialSignalSeq	= *(uint32_t*)signalBuffer.contents;
	task.signalNumber	= signalBuffer.gpuAddress;
	task.waitSignalNumber	= value;

	id<MTLBuffer> taskBuffer = [device newBufferWithBytes:&task
		length:sizeof(struct SignalTask)
		options:MTLResourceStorageModeShared];

	MTL4ArgumentTableDescriptor* argumentsDesc = [MTL4ArgumentTableDescriptor new];
	argumentsDesc.maxBufferBindCount = 1;

	id<MTL4ArgumentTable> arguments = [device newArgumentTableWithDescriptor:argumentsDesc error:nil];
	[arguments setAddress:taskBuffer.gpuAddress atIndex:0];

	[computeEncoder setComputePipelineState:waitPipelines[waitOp]];
	[computeEncoder setArgumentTable:arguments];
	[computeEncoder dispatchThreads:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
	[computeEncoder barrierAfterEncoderStages:MTLStageBlit | MTLStageDispatch
		beforeEncoderStages:MTLStageBlit | MTLStageDispatch
		visibilityOptions:MTL4VisibilityOptionDevice];
}

void gpuSignal(id<MTLDevice> device, id<MTL4ComputeCommandEncoder> computeEncoder, uint32_t value, enum SignalOp signalOp) {
	struct SignalTask task;
	task.signalSeq		= seqBuffer.gpuAddress;
	task.signalNumber	= signalBuffer.gpuAddress;
	task.value		= value;

	id<MTLBuffer> taskBuffer = [device newBufferWithBytes:&task
		length:sizeof(struct WaitTask)
		options:MTLResourceStorageModeShared];

	MTL4ArgumentTableDescriptor* argumentsDesc = [MTL4ArgumentTableDescriptor new];
	argumentsDesc.maxBufferBindCount = 1;

	id<MTL4ArgumentTable> arguments = [device newArgumentTableWithDescriptor:argumentsDesc error:nil];
	[arguments setAddress:taskBuffer.gpuAddress atIndex:0];

	// [computeEncoder barrierAfterEncoderStages:MTLStageBlit | MTLStageDispatch
	// 	beforeEncoderStages:MTLStageBlit | MTLStageDispatch
	// 	visibilityOptions:MTL4VisibilityOptionDevice];
	[computeEncoder setComputePipelineState:signalPipelines[signalOp]];
	[computeEncoder setArgumentTable:arguments];
	[computeEncoder dispatchThreads:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(1, 1, 1)];
}

id<MTLDevice> device;
id<MTL4CommandAllocator> allocator;
id<MTL4CommandAllocator> workerAllocator;

void* secondThread(void* _) {
	@autoreleasepool {

		id<MTL4CommandQueue> queue = [device newMTL4CommandQueue];

		id<MTL4CommandBuffer> commandBuffer = [device newCommandBuffer];
		id<MTL4CommandAllocator> localAllocator = workerAllocator;

		[commandBuffer beginCommandBufferWithAllocator:localAllocator];
		id<MTL4ComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

		gpuSignal(device, computeEncoder, 100, SIGNAL_OP_ATOMIC_MAX);

		[computeEncoder endEncoding];
		[commandBuffer endCommandBuffer];

		usleep(1000000);

		[queue commit:&commandBuffer count:1];

		[localAllocator release];
	}

	return NULL;
}

int main(void) {
	device = MTLCreateSystemDefaultDevice();
	mtl4BeginTracing(device);

	MTL4CompilerDescriptor* compilerDesc = [MTL4CompilerDescriptor new];

	id<MTL4Compiler> compiler = [device newCompilerWithDescriptor:compilerDesc error:nil];

	prepareWaitPipelines(device, compiler);
	prepareSignalPipelines(device, compiler);

	seqBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];
	signalBuffer = [device newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

	id<MTL4CommandQueue> queue = [device newMTL4CommandQueue];
	allocator = [device newCommandAllocator];
	workerAllocator = [device newCommandAllocator];

	pthread_t thread;
	pthread_create(&thread, NULL, secondThread, NULL);

	id<MTL4CommandBuffer> commandBuffer = [device newCommandBuffer];
	[commandBuffer beginCommandBufferWithAllocator:allocator];
	id<MTL4ComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];

	gpuWait(device, computeEncoder, 100, WAIT_OP_EQUAL);

	[computeEncoder endEncoding];
	[commandBuffer endCommandBuffer];

	__block int finished = 0;

	MTL4CommitOptions* options = [MTL4CommitOptions new];
	[options addFeedbackHandler: ^(id<MTL4CommitFeedback>commitFeedback){
		finished = 1;
	}];
	[queue commit:&commandBuffer count:1 options:options];


	while (finished == 0) {}

	mtl4StopTracing();

	pthread_join(thread, NULL);

	[queue release];
	return 0;
}

