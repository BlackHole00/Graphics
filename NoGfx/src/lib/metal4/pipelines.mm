#include "pipelines.h"

#include <dispatch/dispatch.h>

#include <lib/common/heap_allocator.h>
#include <lib/metal4/context.h>

Mtl4PipelineStorage gMtl4PipelineStorage;

void mtl4InitPipelineStorage(GpuResult* result) {
	CmnResult localResult;
	MTL4CompilerDescriptor* compilerDescriptor = nil;

	gMtl4PipelineStorage = {};

	gMtl4PipelineStorage.page = cmnCreatePage(1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4PipelineStorage.arena = cmnPageToArena(gMtl4PipelineStorage.page);

	CmnAllocator allocator = cmnArenaAllocator(&gMtl4PipelineStorage.arena);

	cmnCreateHandleMap(&gMtl4PipelineStorage.pipelines, allocator, {}, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	compilerDescriptor = [MTL4CompilerDescriptor new];
	compilerDescriptor.label = @"No Graphics compiler descriptor";

	gMtl4PipelineStorage.compiler = [gMtl4Context.device newCompilerWithDescriptor:compilerDescriptor error:nil];
	if (gMtl4PipelineStorage.compiler == nil) {
		CMN_SET_RESULT(result, GPU_GENERAL_ERROR);
		goto on_error_cleanup;
		
	}

	[compilerDescriptor release];
	
	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;

on_error_cleanup:
	if (compilerDescriptor != nil) {
		[compilerDescriptor release];
	}

	mtl4FiniPipelineStorage();
}

void mtl4FiniPipelineStorage(void) {
	// TODO: Free all the allocated resources for the pipeline metadata.

	if (gMtl4PipelineStorage.compiler != nil) {
		[gMtl4PipelineStorage.compiler release];
	}

	cmnDestroyPage(gMtl4PipelineStorage.page);

	gMtl4PipelineStorage = {};
}

GpuPipeline mtl4CreateComputePipeline(uint8_t* bytes, size_t size, void* constants, size_t constantsSize, GpuResult* result) {
	CmnResult localResult;
	GpuResult localGpuResult;

	Mtl4PipelineMetadata metadata = {};

	{
		metadata.status	= MTL4_PIPELINE_COMPILING;
		metadata.type	= MTL4_PIPELINE_COMPUTE;

		metadata.constantDataSize = constantsSize;
		metadata.constantData = cmnHeapAlloc<uint8_t>(constantsSize, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}

		metadata.compute.irSize = size;
		metadata.compute.ir = cmnHeapAlloc<uint8_t>(size, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}

		memcpy(metadata.constantData, constants, constantsSize);
		memcpy(metadata.compute.ir, bytes, size);
	}

	{
		metadata.compute.library = mtl4LibraryFromBytes(metadata.compute.ir, size, &localGpuResult);
		if (localGpuResult != GPU_SUCCESS) {
			CMN_SET_RESULT(result, localGpuResult);
			goto on_error_cleanup;
		}

		metadata.compute.function = mtl4LibraryFunctionDescriptorFromLibrary(
			metadata.compute.library,
			metadata.constantData,
			&localGpuResult
		);
		if (localGpuResult != GPU_SUCCESS) {
			CMN_SET_RESULT(result, localGpuResult);
			goto on_error_cleanup;
		}
	}

	Mtl4Pipeline handle;
	{
		CmnScopedStorageSyncLockWrite guard(&gMtl4PipelineStorage.sync);

		handle = cmnInsert(&gMtl4PipelineStorage.pipelines, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return mtl4HandleToGpuPipeline(handle);

on_error_cleanup:
	cmnHeapFree(metadata.constantData);
	cmnHeapFree(metadata.compute.ir);

	if (metadata.compute.library != nil) {
		[metadata.compute.library release];
	}
	if (metadata.compute.function != nil) {
		[metadata.compute.function release];
	}

	return 0;
}

id<MTLLibrary> mtl4LibraryFromBytes(uint8_t* bytes, size_t size, GpuResult* result) {
	dispatch_data_t data = dispatch_data_create(bytes, size, dispatch_get_main_queue(), nil);

	id<MTLLibrary> library = [gMtl4Context.device newLibraryWithData:data error:nil];
	if (library == nil) {
		CMN_SET_RESULT(result, GPU_PIPELINE_IR_VALIDATION_FAILED);
		return nil;
	}

	return library;
}

MTL4FunctionDescriptor* mtl4LibraryFunctionDescriptorFromLibrary(id<MTLLibrary> library, void* constants, GpuResult* result) {
	MTL4LibraryFunctionDescriptor* baseFunction = [MTL4LibraryFunctionDescriptor new];
	baseFunction.library = library;
	baseFunction.name = @"main";

	if (constants == nullptr) {
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return baseFunction;
	}

	MTLFunctionConstantValues* constantValues = [MTLFunctionConstantValues new];
	[constantValues setConstantValue:constants type:MTLDataTypeStruct atIndex:0];

	MTL4SpecializedFunctionDescriptor* function = [MTL4SpecializedFunctionDescriptor new];
	function.functionDescriptor = baseFunction;
	function.constantValues = constantValues;

	[constantValues release];
	[baseFunction release];

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return function;
}

