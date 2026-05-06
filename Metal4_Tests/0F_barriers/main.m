#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <objc/runtime.h>

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

	FILE* f = fopen("image.png", "rb");
	if (f == NULL) {
		printf("Could not open image.png\n");
		return -1;
	}

	int x, y, channels;
	uint8_t* data = stbi_load_from_file(f, &x, &y, &channels, 4);
	assert(channels == 4);

	fclose(f);

	/* Use a 256-byte-aligned row pitch and blit encoders for buffer<->texture copies.
	   Many Metal copy APIs require a pitched row stride (alignment) when copying
	   between buffers and textures. Failing to use the required alignment will
	   produce corrupted results (often visible as fully transparent or white).
	*/

	size_t bytesPerRow = x * 4;
	const size_t kRowAlignment = 256; // bytesPerRow alignment required by GPU
	size_t alignedBytesPerRow = ((bytesPerRow + kRowAlignment - 1) / kRowAlignment) * kRowAlignment;
	size_t pitchedBufferSize = alignedBytesPerRow * y;

	id<MTLBuffer> uploadBuffer = [device newBufferWithLength:pitchedBufferSize options:MTLResourceStorageModeShared];
	id<MTLBuffer> downloadBuffer = [device newBufferWithLength:pitchedBufferSize options:MTLResourceStorageModeShared];

	// Copy rows into the pitched upload buffer (pad each row to the aligned stride).
	uint8_t* uploadPtr = (uint8_t*)uploadBuffer.contents;
	memset(uploadPtr, 0, pitchedBufferSize);
	for (int row = 0; row < y; ++row) {
		memcpy(uploadPtr + row * alignedBytesPerRow, data + row * bytesPerRow, bytesPerRow);
	}

	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
		width:x
		height:y
		mipmapped:false];
	textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
	// Make texture shared so CPU/GPU have coherent access on platforms where
    // blit copy support via MTL4 compute encoders may require shared storage.
	textureDescriptor.storageMode = MTLStorageModeShared;
	id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];

	id<MTL4CommandQueue> queue = [device newMTL4CommandQueue];
	id<MTL4CommandAllocator> allocator = [device newCommandAllocator];

	id<MTL4CommandBuffer> commandBuffer = [device newCommandBuffer];
	[commandBuffer beginCommandBufferWithAllocator:allocator];

	// Diagnostic: print command buffer class methods that mention "blit"/"copy"/"encoder".
	Class cbClass = object_getClass((id)commandBuffer);
	unsigned int mcount = 0;
	Method *mlist = class_copyMethodList(cbClass, &mcount);
	printf("MTL4CommandBuffer class %s methods=%u\n", class_getName(cbClass), mcount);
	for (unsigned int mi = 0; mi < mcount; ++mi) {
		SEL s = method_getName(mlist[mi]);
		const char *n = sel_getName(s);
		printf("  %s\n", n);
	}
	free(mlist);

	// Also inspect the compute encoder class returned by MTL4 command buffer.
	id tmpEnc = [commandBuffer computeCommandEncoder];
	if (tmpEnc != nil) {
		Class encClass = object_getClass((id)tmpEnc);
		unsigned int ecount = 0;
		Method *elist = class_copyMethodList(encClass, &ecount);
		printf("MTL4ComputeCommandEncoder class %s methods=%u\n", class_getName(encClass), ecount);
		for (unsigned int ei = 0; ei < ecount; ++ei) {
			SEL s = method_getName(elist[ei]);
			printf("  %s\n", sel_getName(s));
		}
		free(elist);
		[tmpEnc endEncoding];
	}

	// Implement the requested sequence:
	// 1) blit upload copy (buffer -> texture)
	// 2) blit->dispatch barrier
	// 3) dispatch a no-op compute kernel
	// 4) dispatch->blit barrier
	// 5) blit download copy (texture -> buffer)

	id<MTLFence> fence = [device newFence];

	// Compile a tiny no-op compute kernel.
	const char *nopSrc =
		"#include <metal_stdlib>\n"
		"using namespace metal;\n"
		"kernel void nop(uint2 gid [[thread_position_in_grid]]) { }\n";
	NSError *err = nil;
	NSString *src = [NSString stringWithUTF8String:nopSrc];
	id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
	if (lib == nil) {
		printf("Failed to compile nop kernel: %s\n", [[err localizedDescription] UTF8String]);
		mtl4StopTracing();
		return -1;
	}
	id<MTLFunction> fnNop = [lib newFunctionWithName:@"nop"];
	id<MTLComputePipelineState> pipeNop = [device newComputePipelineStateWithFunction:fnNop error:&err];
	if (pipeNop == nil) {
		printf("Failed to create compute pipeline for nop: %s\n", [[err localizedDescription] UTF8String]);
		mtl4StopTracing();
		return -1;
	}

	id<MTL4CommandBuffer> cmd = [device newCommandBuffer];
	[cmd beginCommandBufferWithAllocator:allocator];

	// 1) blit upload copy
	id<MTL4ComputeCommandEncoder> encA = [cmd computeCommandEncoder];
	[encA copyFromBuffer:uploadBuffer
		sourceOffset:0
		sourceBytesPerRow:alignedBytesPerRow
		sourceBytesPerImage:pitchedBufferSize
		sourceSize:MTLSizeMake(x, y, 1)
		toTexture:texture
		destinationSlice:0
		destinationLevel:0
		destinationOrigin:MTLOriginMake(0, 0, 0)
		options:0];

	// 2) blit -> dispatch barrier
	[encA barrierAfterEncoderStages:UINT64_MAX beforeEncoderStages:UINT64_MAX options:0];
	[encA endEncoding];

	// 3) dispatch a no-op compute kernel
	id<MTL4ComputeCommandEncoder> encCompute = [cmd computeCommandEncoder];
	[encCompute setComputePipelineState:pipeNop];
	MTLSize threadsPerGrid = MTLSizeMake(1, 1, 1);
	MTLSize threadsPerThreadgroup = MTLSizeMake(1, 1, 1);
	[encCompute dispatchThreads:threadsPerGrid threadsPerThreadgroup:threadsPerThreadgroup];

	// 4) dispatch -> blit barrier
	[encCompute barrierAfterEncoderStages:UINT64_MAX beforeEncoderStages:UINT64_MAX options:0];
	[encCompute endEncoding];

	// 5) blit download copy
	id<MTL4ComputeCommandEncoder> encB = [cmd computeCommandEncoder];
	[encB copyFromTexture:texture
		sourceSlice:0
		sourceLevel:0
		sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:MTLSizeMake(x, y, 1)
		toBuffer:downloadBuffer
		destinationOffset:0
		destinationBytesPerRow:alignedBytesPerRow
		destinationBytesPerImage:pitchedBufferSize
		options:0];
	[encB endEncoding];

	[cmd endCommandBuffer];

	__block int finished = 0;
	MTL4CommitOptions* commitOpts = [MTL4CommitOptions new];
	[commitOpts addFeedbackHandler:^(id<MTL4CommitFeedback> commitFeedback){ finished = 1; }];
	[queue commit:&cmd count:1 options:commitOpts];
	while (finished == 0) {}

	// Unpack the pitched download buffer into a contiguous RGBA image for PNG writing.
	uint8_t* packedOut = malloc(x * y * 4);
	uint8_t* downloadPtr = (uint8_t*)downloadBuffer.contents;
	for (int row = 0; row < y; ++row) {
		memcpy(packedOut + row * bytesPerRow, downloadPtr + row * alignedBytesPerRow, bytesPerRow);
	}

	stbi_write_png("out.png", x, y, 4, packedOut, bytesPerRow);

	// Also write the original CPU image for visual comparison.
	stbi_write_png("cpu_out.png", x, y, 4, data, bytesPerRow);

	// Diagnostics: compare the GPU-produced image with the original image bytes.
	int totalBytes = x * y * 4;
	int mismatches = 0;
	int firstMismatch = -1;
	int alphaZero = 0;
	for (int i = 0; i < totalBytes; ++i) {
		if (packedOut[i] != data[i]) {
			mismatches++;
			if (firstMismatch == -1) firstMismatch = i;
		}
	}
	for (int p = 0; p < x * y; ++p) {
		if (packedOut[p * 4 + 3] == 0) alphaZero++;
	}

	printf("GPU vs CPU mismatches: %d / %d bytes\n", mismatches, totalBytes);
	printf("GPU alpha==0 pixels: %d / %d\n", alphaZero, x * y);
	if (firstMismatch != -1) {
		int pix = firstMismatch / 4;
		int row = pix / x;
		int col = pix % x;
		printf("first mismatch at byte index=%d (pixel=%d col=%d row=%d)\n", firstMismatch, pix, col, row);
		printf("original RGBA=%u %u %u %u\n",
			(unsigned)data[firstMismatch], (unsigned)data[firstMismatch+1], (unsigned)data[firstMismatch+2], (unsigned)data[firstMismatch+3]);
		printf("gpu      RGBA=%u %u %u %u\n",
			(unsigned)packedOut[firstMismatch], (unsigned)packedOut[firstMismatch+1], (unsigned)packedOut[firstMismatch+2], (unsigned)packedOut[firstMismatch+3]);
	}

	free(packedOut);
	stbi_image_free(data);

	mtl4StopTracing();
	return 0;
}

