#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <objc/runtime.h>

#include <Metal/Metal.h>

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

	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
		width:640
		height:480
		mipmapped:false];
	textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
	textureDescriptor.storageMode = MTLStorageModeShared;

	MTLSizeAndAlign a = [device heapTextureSizeAndAlignWithDescriptor:textureDescriptor];

	textureDescriptor.storageMode = MTLStorageModePrivate;
	MTLSizeAndAlign b = [device heapTextureSizeAndAlignWithDescriptor:textureDescriptor];

	printf("a: %lu %lu\n", a.align, a.size);
	printf("b: %lu %lu\n", b.align, b.size);

	mtl4StopTracing();
	return 0;
}

