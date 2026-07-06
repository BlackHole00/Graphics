package main

import "core:slice/heap"
import "core:container/xar"
import "core:image"
import "core:log"
import "core:mem"
import "core:sync"
import hm "core:container/handle_map"
import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

R_Handle :: struct {
	gen: u32,
	idx: u32,
}

R_Texture :: struct {
	handle:		R_Texture_Handle,

	texture:	^MTL.Texture,
	status:		R_Texture_Status,
}
R_Texture_Handle :: distinct R_Handle

R_Texture_Status :: enum {
	Loading,
	Loaded,
	Load_Failed,
}

R_Gpu_References_Header :: struct {
	textures:	uintptr,
}

R_Resources :: struct {
	heap:			^MTL.Heap,

	textures:		hm.Static_Handle_Map(1024, R_Texture, R_Texture_Handle),
	textures_mutex:		sync.Mutex,

	default_texture:	^R_Texture,

	gpu_references:		^MTL.Buffer,
	texture_references:	^MTL.Buffer,
}
r_resources: R_Resources

r_init_resource_storage :: proc() {
	heap_desc := MTL.HeapDescriptor.new()->autorelease()
	heap_desc->setStorageMode(.Shared)
	heap_desc->setResourceOptions({})
	heap_desc->setSize(25 * mem.Megabyte)

	r_resources.heap = r_renderer.device->newHeapWithDescriptor(heap_desc)
	log.assertf(r_resources.heap != nil, "Could not create a MTLHeap. Aborting.")

	r_resources.gpu_references = r_resources.heap->newBufferWithLength(
		size_of(R_Gpu_References_Header),
		{},
	)
	r_resources.gpu_references->setLabel(c_AT("r_resources.gpu_references"))

	r_resources.texture_references = r_resources.heap->newBufferWithLength(
		1024 * size_of(MTL.ResourceID),
		{},
	)
	r_resources.texture_references->setLabel(c_AT("r_resources.texture_references"))

	default_texture_handle := r_register_texture_from_bytes(
		mem.slice_to_bytes(R_DEFAULT_TEXTURE[:]),
		.RGBA8Unorm,
		2,
		2,
		false,
		"Default texture",
	)
	r_resources.default_texture = r_get_resource(default_texture_handle)
}

r_register_texture_from_file :: proc(filename: string, format: MTL.PixelFormat, mipmapped: bool) -> R_Texture_Handle {
	image, err := image.load_from_file(filename, allocator = context.temp_allocator)
	log.assertf(err == nil, "Could not load image `%s`: Got error `%v`. Aborting.", filename, err)

	log.assertf(image.channels == r_get_bytes_per_pixel(format), "Incompatible image data and image format")

	return r_register_texture_from_bytes(
		image.pixels.buf[:],
		format,
		image.width,
		image.height,
		false,
		filename,
	)
}

r_sync_resource_storage_gpu_references :: proc() {
	header_cpu_ptr := cast(uintptr)r_resources.gpu_references->contents()
	header_ptr := cast(^R_Gpu_References_Header)header_cpu_ptr

	textures_cpu_ptr := cast(uintptr)r_resources.texture_references->contents()
	textures_gpu_ptr := cast(uintptr)r_resources.texture_references->gpuAddress()
	texture_array_ptr := cast([^]MTL.ResourceID)(textures_cpu_ptr)

	texture_array_ptr[0] = r_resources.default_texture.texture->gpuResourceID()

	for i := 0; i < 1024; i += 1 {
		texture := &r_resources.textures.items[i]

		if texture.handle.idx == 0 {
			texture_array_ptr[i] = r_resources.default_texture.texture->gpuResourceID()
		} else {
			texture_array_ptr[i] = texture.texture->gpuResourceID()
		}
	}

	header_ptr.textures = textures_gpu_ptr
}

r_register_texture_from_bytes :: proc(
	data:			[]byte,
	format:			MTL.PixelFormat,
	width:			int,
	height:			int,
	mipmapped:		bool,
	label:			string,
) -> (handle: R_Texture_Handle) {
	descriptor := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(
		auto_cast format,
		cast(NS.UInteger)width,
		cast(NS.UInteger)height,
		mipmapped
	)
	descriptor->setStorageMode(.Shared)

	metal_texture := r_resources.heap->newTextureWithDescriptor(descriptor)
	metal_texture->replaceRegion(
		MTL.Region {
			origin = { 0, 0, 0 },
			size   = { cast(NS.UInteger)width, cast(NS.UInteger)height, 1 },
		},
		0,
		raw_data(data),
		cast(NS.UInteger)(width * r_get_bytes_per_pixel(format))
	)
	metal_texture->setLabel(
		NS.String.alloc()->initWithBytes(
			raw_data(label),
			auto_cast len(label),
			NS.UTF8StringEncoding,
		)->autorelease())

	texture: R_Texture
	texture.texture = metal_texture

	if sync.mutex_guard(&r_resources.textures_mutex) {
		ok: bool
		handle, ok = hm.add(&r_resources.textures, texture)

		log.assertf(ok, "Could not create a texture. Aborting.")
	}

	return handle
}

r_get_bytes_per_pixel :: proc(format: MTL.PixelFormat) -> int {
	bytes_count := R_BYTES_PER_PIXEL[format]
	log.assertf(bytes_count != 0, "Invalid pixel format found %v", format)

	return bytes_count
}

r_get_texture :: proc(handle: R_Texture_Handle) -> (texture: ^R_Texture, ok: bool) #optional_ok {
	sync.mutex_guard(&r_resources.textures_mutex)
	texture, ok = hm.get(&r_resources.textures, handle)

	if !ok {
		texture = r_resources.default_texture
	}

	return
}

r_get_resource :: proc{
	r_get_texture,
}


@(rodata)
R_BYTES_PER_PIXEL := #sparse[MTL.PixelFormat]int{
	.Unspecialized		= 0,
	.Invalid		= 0,
	.A8Unorm		= 1,
	.R8Unorm		= 1,
	.R8Unorm_sRGB		= 1,
	.R8Snorm		= 1,
	.R8Uint			= 1,
	.R8Sint			= 1,
	.R16Unorm		= 2,
	.R16Snorm		= 2,
	.R16Uint		= 2,
	.R16Sint		= 2,
	.R16Float		= 2,
	.RG8Unorm		= 2,
	.RG8Unorm_sRGB		= 2,
	.RG8Snorm		= 2,
	.RG8Uint		= 2,
	.RG8Sint		= 2,
	.B5G6R5Unorm		= 2,
	.A1BGR5Unorm		= 2,
	.ABGR4Unorm		= 2,
	.BGR5A1Unorm		= 2,
	.R32Uint		= 4,
	.R32Sint		= 4,
	.R32Float		= 4,
	.RG16Unorm		= 4,
	.RG16Snorm		= 4,
	.RG16Uint		= 4,
	.RG16Sint		= 4,
	.RG16Float		= 4,
	.RGBA8Unorm		= 4,
	.RGBA8Unorm_sRGB	= 4,
	.RGBA8Snorm		= 4,
	.RGBA8Uint		= 4,
	.RGBA8Sint		= 4,
	.BGRA8Unorm		= 4,
	.BGRA8Unorm_sRGB	= 4,
	.RGB10A2Unorm		= 4,
	.RGB10A2Uint		= 4,
	.RG11B10Float		= 4,
	.RGB9E5Float		= 4,
	.BGR10A2Unorm		= 4,
	.RG32Uint		= 8,
	.RG32Sint		= 8,
	.RG32Float		= 8,
	.RGBA16Unorm		= 8,
	.RGBA16Snorm		= 8,
	.RGBA16Uint		= 8,
	.RGBA16Sint		= 8,
	.RGBA16Float		= 8,
	.RGBA32Uint		= 16,
	.RGBA32Sint		= 16,
	.RGBA32Float		= 16,
	.GBGR422		= 2,
	.BGRG422		= 2,
	.Depth16Unorm		= 2,
	.Depth32Float		= 4,
	.Stencil8		= 1,
	.Depth24Unorm_Stencil8	= 4,
	.Depth32Float_Stencil8	= 4,
	.X32_Stencil8		= 4,
	.X24_Stencil8		= 3,
	.BGR10_XR		= 0,
	.BGR10_XR_sRGB		= 0,
	.BGRA10_XR		= 0,
	.BGRA10_XR_sRGB		= 0,
	.BC1_RGBA		= 0,
	.BC1_RGBA_sRGB		= 0,
	.BC2_RGBA		= 0,
	.BC2_RGBA_sRGB		= 0,
	.BC3_RGBA		= 0,
	.BC3_RGBA_sRGB		= 0,
	.BC4_RUnorm		= 0,
	.BC4_RSnorm		= 0,
	.BC5_RGUnorm		= 0,
	.BC5_RGSnorm		= 0,
	.BC6H_RGBFloat		= 0,
	.BC6H_RGBUfloat		= 0,
	.BC7_RGBAUnorm		= 0,
	.BC7_RGBAUnorm_sRGB	= 0,
	.PVRTC_RGB_2BPP		= 0,
	.PVRTC_RGB_2BPP_sRGB	= 0,
	.PVRTC_RGB_4BPP		= 0,
	.PVRTC_RGB_4BPP_sRGB	= 0,
	.PVRTC_RGBA_2BPP	= 0,
	.PVRTC_RGBA_2BPP_sRGB	= 0,
	.PVRTC_RGBA_4BPP	= 0,
	.PVRTC_RGBA_4BPP_sRGB	= 0,
	.EAC_R11Unorm		= 0,
	.EAC_R11Snorm		= 0,
	.EAC_RG11Unorm		= 0,
	.EAC_RG11Snorm		= 0,
	.EAC_RGBA8		= 0,
	.EAC_RGBA8_sRGB		= 0,
	.ETC2_RGB8		= 0,
	.ETC2_RGB8_sRGB		= 0,
	.ETC2_RGB8A1		= 0,
	.ETC2_RGB8A1_sRGB	= 0,
	.ASTC_4x4_sRGB		= 0,
	.ASTC_5x4_sRGB		= 0,
	.ASTC_5x5_sRGB		= 0,
	.ASTC_6x5_sRGB		= 0,
	.ASTC_6x6_sRGB		= 0,
	.ASTC_8x5_sRGB		= 0,
	.ASTC_8x6_sRGB		= 0,
	.ASTC_8x8_sRGB		= 0,
	.ASTC_10x5_sRGB		= 0,
	.ASTC_10x6_sRGB		= 0,
	.ASTC_10x8_sRGB		= 0,
	.ASTC_10x10_sRGB	= 0,
	.ASTC_12x10_sRGB	= 0,
	.ASTC_12x12_sRGB	= 0,
	.ASTC_4x4_LDR		= 0,
	.ASTC_5x4_LDR		= 0,
	.ASTC_5x5_LDR		= 0,
	.ASTC_6x5_LDR		= 0,
	.ASTC_6x6_LDR		= 0,
	.ASTC_8x5_LDR		= 0,
	.ASTC_8x6_LDR		= 0,
	.ASTC_8x8_LDR		= 0,
	.ASTC_10x5_LDR		= 0,
	.ASTC_10x6_LDR		= 0,
	.ASTC_10x8_LDR		= 0,
	.ASTC_10x10_LDR		= 0,
	.ASTC_12x10_LDR		= 0,
	.ASTC_12x12_LDR		= 0,
	.ASTC_4x4_HDR		= 0,
	.ASTC_5x4_HDR		= 0,
	.ASTC_5x5_HDR		= 0,
	.ASTC_6x5_HDR		= 0,
	.ASTC_6x6_HDR		= 0,
	.ASTC_8x5_HDR		= 0,
	.ASTC_8x6_HDR		= 0,
	.ASTC_8x8_HDR		= 0,
	.ASTC_10x5_HDR		= 0,
	.ASTC_10x6_HDR		= 0,
	.ASTC_10x8_HDR		= 0,
	.ASTC_10x10_HDR		= 0,
	.ASTC_12x10_HDR		= 0,
	.ASTC_12x12_HDR		= 0,
}

@(require) import "core:image/bmp"
@(require) import "core:image/netpbm"
@(require) import "core:image/png"
@(require) import "core:image/qoi"
@(require) import "core:image/tga"
@(require) import "core:image/jpeg"


_ :: bmp
_ :: netpbm
_ :: png
_ :: qoi
_ :: tga
_ :: jpeg

