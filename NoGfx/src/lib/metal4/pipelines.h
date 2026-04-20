#ifndef MTL4_PIPELINE_H
#define MTL4_PIPELINE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/storage_sync.h>

typedef CmnHandle Mtl4Pipeline;

typedef enum Mtl4PipelineStatus {
	MTL4_PIPELINE_COMPILING,
	MTL4_PIPELINE_READY,
} Mtl4PipelineStatus;

typedef enum Mtl4PipelineType {
	MTL4_PIPELINE_COMPUTE,
	MTL4_PIPELINE_GRAPHICS,
	MTL4_PIPELINE_MESHLET,
} Mtl4PipelineType;

typedef struct Mtl4GraphicsPipelineMetadata {
	id<MTLLibrary>			vertexLibrary;
	id<MTLLibrary>			fragmentLibrary;

	MTL4FunctionDescriptor*	vertexFunction;
	MTL4FunctionDescriptor*	fragmentFunction;

	uint8_t*	vertexIr;
	size_t		vertexIrSize;
	uint8_t*	fragmentIr;
	size_t		fragmentIrSize;
} Mtl4GraphicsPipelineMetadata;

typedef struct Mtl4ComputePipelineMetadata {
	id<MTLLibrary>	library;

	MTL4FunctionDescriptor*	function;

	uint8_t*	ir;
	size_t		irSize;
} Mtl4ComputePipelineMetadata;

typedef struct Mtl4MeshletPipelineMetadata {
	
} Mtl4MeshletPipelineMetadata;

typedef struct Mtl4PipelineMetadata {
	Mtl4PipelineStatus	status;
	Mtl4PipelineType	type;

	uint8_t*		constantData;
	size_t			constantDataSize;

	union {
		Mtl4ComputePipelineMetadata	compute;
		Mtl4GraphicsPipelineMetadata	graphics;
		Mtl4MeshletPipelineMetadata	meshlet;
	};
} Mtl4PipelineMetadata;

typedef struct Mtl4PipelineStorage {
	CmnPage		page;
	CmnArena	arena;

	id<MTL4Compiler>	compiler;

	CmnStorageSync	sync;
	CmnHandleMap<Mtl4PipelineMetadata> pipelines;
} Mtl4PipelineStorage;
extern Mtl4PipelineStorage gMtl4PipelineStorage;

void mtl4InitPipelineStorage(GpuResult* result);
void mtl4FiniPipelineStorage(void);

GpuPipeline mtl4CreateComputePipeline(uint8_t* bytes, size_t size, void* constants, size_t constantsSize, GpuResult* result);
// GpuPipeline mtl4CreateRenderPipeline(uint8_t* bytes, size_t size, GpuResult* result);
// GpuPipeline mtl4CreateMeshletPipeline(uint8_t* bytes, size_t size, GpuResult* result);
void mtl4FreePipeline(GpuPipeline pipeline);

id<MTLLibrary> mtl4LibraryFromBytes(uint8_t* bytes, size_t size, GpuResult* result);
MTL4FunctionDescriptor* mtl4LibraryFunctionDescriptorFromLibrary(id<MTLLibrary> library, void* constants, GpuResult* result);

inline Mtl4Pipeline mtl4GpuPipelineToHandle(GpuPipeline pipeline) {
	return *(Mtl4Pipeline*)&pipeline;
}

inline GpuPipeline mtl4HandleToGpuPipeline(Mtl4Pipeline handle) {
	return *(GpuPipeline*)&handle;
}

#endif // MTL4_PIPELINE_H

