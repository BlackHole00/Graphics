package main

import "core:slice/heap"
import "core:image"
import "core:log"
import "core:mem"
import sa "core:container/small_array"
import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

R_MAX_TEXTURES	:: 1024
R_MAX_MESHES	:: 1024
R_MAX_MATERIALS	:: 1024

R_Texture_Handle :: distinct u64
R_Texture :: struct {
	handle:			R_Texture_Handle,
	metal_texture:		^MTL.Texture,
	needs_mipmapping:	bool,
}

R_Material_Texture :: enum {
	Base_Color,
	Metallic,
	Roughness,
	Normal,
	Ambient_Occlusion,
}

R_Material_Handle :: distinct u64
R_Material :: struct {
	handle:		R_Material_Handle,
	buffer:		^MTL.Buffer,
}
R_Material_Gpu_Repr :: struct #packed {
	textures:	[R_Material_Texture]MTL.ResourceID,
}

R_Mesh_Handle :: distinct u64
// TODO: Handle submeshes
R_Mesh :: struct {
	handle:		R_Mesh_Handle,

	vertex_count:	uint,
	index_count:	uint,

	// Follows the layout:
	//	- size_of(R_Mesh_Gpu_Repr) bytes: header
	//	- size_of(R_Vertex) * vertex_count bytes: vertices.
	//	- size_of(u32) * index_count: indices
	buffer:		^MTL.Buffer,
}
R_Mesh_Gpu_Repr :: struct #packed {
	vertex_buffer:	MTL.GPUAddress,
	index_buffer:	MTL.GPUAddress,
}

R_Resources :: struct {
	residency_set:	^MTL.ResidencySet,
	heap:		^MTL.Heap,

	textures:	sa.Small_Array(R_MAX_TEXTURES,	R_Texture),
	meshes:		sa.Small_Array(R_MAX_MESHES,	R_Mesh),
	materials:	sa.Small_Array(R_MAX_MATERIALS,	R_Material),

	meshes_gpu_buffer:	^MTL.Buffer,
	materials_gpu_buffer:	^MTL.Buffer,
	resources_gpu_buffer:	^MTL.Buffer,

	last_mipmapped_texture:	R_Texture_Handle,
}
r_resources: R_Resources

r_init_resource_storage :: proc() -> R_Result {
	error: ^NS.Error

	residency_set_descriptor := MTL.ResidencySetDescriptor.new()->autorelease()
	residency_set_descriptor->setLabel(c_AT("r_resources.residency_set"))
	r_resources.residency_set = r_renderer.device->newResidencySetWithDescriptor(
		residency_set_descriptor,
		&error,
	)
	if r_resources.residency_set == nil {
		log.error("Could not create a residency set.")
		return .Resource_Creation_Failed
	}

	heap_descriptor := MTL.HeapDescriptor.new()->autorelease()
	heap_descriptor->setSize(64 * mem.Megabyte)
	heap_descriptor->setStorageMode(.Shared)
	heap_descriptor->setHazardTrackingMode(.Untracked)
	heap_descriptor->setResourceOptions({})

	r_resources.heap = r_renderer.device->newHeapWithDescriptor(heap_descriptor)
	if r_resources.heap == nil {
		log.error("Could not create a MTLHeap.")
		return .Out_Of_Gpu_Memory
	}
	r_resources.heap->setLabel(c_AT("r_resources.heap"))

	r_resources.residency_set->addAllocation(r_resources.heap)
	r_resources.residency_set->commit()

	default_mesh, default_mesh_res := r_register_mesh(R_CUBE_VERTICES[:], R_CUBE_INDICES[:], "Default Mesh")
	if default_mesh_res != nil {
		log.errorf("Could not create the default mesh: Got result `%v`.", default_mesh_res)
		return default_mesh_res
	}
	assert(default_mesh == 0)

	default_texture, default_texture_res := r_register_texture_from_bytes(
		mem.slice_to_bytes(R_DEFAULT_TEXTURE[:]),
		.RGBA8Unorm,
		2,
		2,
		true,
		"Default Texture",
	)
	if default_texture_res != nil {
		log.errorf("Could not create the default texture: Got result `%v`.", default_texture_res)
		return default_texture_res
	}
	assert(default_texture == 0)

	default_material, default_material_res := r_register_material(
		{},
		"Default Material",
	)
	if default_material_res != nil {
		log.errorf("Could not create the default material: Got result `%v`.", default_material_res)
		return default_material_res
	}
	assert(default_material == 0)

	return nil
}

r_register_mesh :: proc(
	vertices: []R_Vertex,
	indices: []u32,
	label := "",
) -> (handle: R_Mesh_Handle, res: R_Result) {

	if sa.len(r_resources.meshes) == R_MAX_MATERIALS {
		return 0, .Out_Of_Resource_Slots
	}

	handle = cast(R_Mesh_Handle)sa.len(r_resources.meshes)

	total_buffer_size := size_of(R_Mesh_Gpu_Repr) + len(vertices) * size_of(R_Vertex) + len(indices) * size_of(u32)
	mesh_buffer := r_resources.heap->newBufferWithLength(
		cast(NS.UInteger)total_buffer_size,
		{},
	)
	if mesh_buffer == nil {
		log.error("Could not allocate a mesh. Out of memory.")
		return 0, .Out_Of_Gpu_Memory
	}

	if label != "" {
		str := NS.String.alloc()->initWithBytes(
			raw_data(label),
			cast(NS.UInteger)len(label),
			NS.UTF8StringEncoding,
		)->autorelease()
		mesh_buffer->setLabel(str)
	}

	gpu_address	:= mesh_buffer->gpuAddress()
	gpu_vertices	:= gpu_address + size_of(R_Mesh_Gpu_Repr)
	gpu_indices	:= gpu_address + size_of(R_Mesh_Gpu_Repr) + cast(MTL.GPUAddress)len(vertices) * size_of(R_Vertex)

	contents	:= cast(uintptr)mesh_buffer->contents()
	header		:= cast(^R_Mesh_Gpu_Repr)contents
	vertices_ptr	:= cast([^]R_Vertex)(contents + size_of(R_Mesh_Gpu_Repr))
	indices_ptr	:= cast([^]u32)(contents + size_of(R_Mesh_Gpu_Repr) + cast(uintptr)len(vertices) * size_of(R_Vertex))

	header.vertex_buffer = gpu_vertices
	header.index_buffer = gpu_indices
	mem.copy_non_overlapping(vertices_ptr, raw_data(vertices), len(vertices) * size_of(R_Vertex))
	mem.copy_non_overlapping(indices_ptr, raw_data(indices), len(indices) * size_of(u32))

	mesh := R_Mesh {
		handle		= handle,
		vertex_count	= len(vertices),
		index_count	= len(indices),
		buffer		= mesh_buffer,
	}
	sa.append(&r_resources.meshes, mesh)
	
	return handle, nil
}

r_register_texture_from_file :: proc(
	filename: string,
	format: MTL.PixelFormat,
	mipmapped := true,
) -> (handle: R_Texture_Handle, res: R_Result) {

	if sa.len(r_resources.textures) == R_MAX_TEXTURES {
		return 0, .Out_Of_Resource_Slots
	}

	img, err := image.load_from_file(filename, allocator=context.temp_allocator)
	if err != nil {
		log.errorf("Could not open the texture file `%s`: Got error `%v`.", filename, err)
		return 0, .Could_Not_Open_Resource_File
	}

	if img.channels == 3 {
		unimplemented("TODO: support 3 channels images: They do not have a direct Metal counterpart.")
	}

	if img.channels != r_get_bytes_per_pixel(format) {
		log.errorf(
			"Could not register texture the texture `%v`: The image format does not have the expected " +
			"number of channels."
		)
		return 0, .Incongruent_Resource_Description
	}

	return r_register_texture_from_bytes(
		img.pixels.buf[:],
		format,
		img.width,
		img.height,
		mipmapped,
		filename,
	)
}

r_register_texture_from_bytes :: proc(
	data:			[]byte,
	format:			MTL.PixelFormat,
	width:			int,
	height:			int,
	mipmapped := true,
	label := "",
) -> (handle: R_Texture_Handle, res: R_Result) {

	if sa.len(r_resources.textures) == R_MAX_TEXTURES {
		return 0, .Out_Of_Resource_Slots
	}

	handle = cast(R_Texture_Handle)sa.len(r_resources.textures)

	texture_descriptor := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(
		format,
		cast(NS.UInteger)width,
		cast(NS.UInteger)height,
		mipmapped,
	)
	texture_descriptor->setStorageMode(.Shared)
	texture_descriptor->setUsage(.ShaderRead)

	metal_texture := r_resources.heap->newTextureWithDescriptor(texture_descriptor)
	if metal_texture == nil {
		log.error("Could not allocate a texture: Out of memory.")
		return 0, .Out_Of_Gpu_Memory
	}

	if label != "" {
		str := NS.String.alloc()->initWithBytes(
			raw_data(label),
			cast(NS.UInteger)len(label),
			NS.UTF8StringEncoding,
		)->autorelease()
		metal_texture->setLabel(str)
	}

	metal_texture->replaceRegion_mipmapLevel_withBytes_bytesPerRow(
		MTL.Region {
			origin = { 0, 0, 0 },
			size = { cast(NS.UInteger)width, cast(NS.UInteger)height, 1 },
		},
		0,
		raw_data(data),
		cast(NS.UInteger)(r_get_bytes_per_pixel(format) * width),
	)

	texture := R_Texture {
		handle			= handle,
		metal_texture		= metal_texture,
		needs_mipmapping	= mipmapped,
	}
	sa.append(&r_resources.textures, texture)

	return handle, nil
}

r_register_material :: proc(
	texture_set: [R_Material_Texture]R_Texture_Handle,
	label := "",
) -> (handle: R_Material_Handle, res: R_Result) {

	if sa.len(r_resources.materials) == R_MAX_MATERIALS {
		return 0, .Out_Of_Resource_Slots
	}

	handle = cast(R_Material_Handle)sa.len(r_resources.materials)

	buffer := r_resources.heap->newBufferWithLength(
		size_of(R_Material_Gpu_Repr),
		{},
	)
	if label != "" {
		str := NS.String.alloc()->initWithBytes(
			raw_data(label),
			cast(NS.UInteger)len(label),
			NS.UTF8StringEncoding,
		)->autorelease()
		buffer->setLabel(str)
	}

	buffer_contents := cast(^R_Material_Gpu_Repr)buffer->contents()

	for texture_handle, type in texture_set {
		buffer_contents.textures[type] = r_gpu_handle_of(texture_handle)
	}

	material := R_Material {
		handle = handle,
		buffer = buffer,
	}
	sa.append(&r_resources.materials, material)

	return handle, nil
}

r_is_mesh_valid :: proc(mesh: R_Mesh_Handle) -> bool {
	return mesh < cast(R_Mesh_Handle)sa.len(r_resources.meshes)
}

r_is_texture_valid :: proc(texture: R_Texture_Handle) -> bool {
	return texture < cast(R_Texture_Handle)sa.len(r_resources.textures)
}

r_is_material_valid :: proc(material: R_Material_Handle) -> bool {
	return material < cast(R_Material_Handle)sa.len(r_resources.materials)
}

r_gpu_handle_of_mesh :: proc(mesh: R_Mesh_Handle) -> (MTL.GPUAddress, bool) #optional_ok {
	return r_get_resource(mesh).buffer->gpuAddress(), true
}

r_gpu_handle_of_texture :: proc(texture: R_Texture_Handle) -> (MTL.ResourceID, bool) #optional_ok {
	return r_get_resource(texture).metal_texture->gpuResourceID(), true
}

r_gpu_handle_of_material :: proc(material: R_Material_Handle) -> (MTL.GPUAddress, bool) #optional_ok {
	return r_get_resource(material).buffer->gpuAddress(), true
}

r_sync_resource_storage_gpu_references :: proc() {}

r_generate_mipmaps_for_textures :: proc() {
	if cast(int)r_resources.last_mipmapped_texture == sa.len(r_resources.textures) {
		return
	}

	// TODO: Only check for new textures (and do not create a command encoder if there are no new ones)

	command := r_renderer.frame_command_buffer->computeCommandEncoder()

	for &texture in sa.slice(&r_resources.textures)[r_resources.last_mipmapped_texture:] {
		if !texture.needs_mipmapping {
			continue
		}

		command->generateMipmapsForTexture(texture.metal_texture)
		texture.needs_mipmapping = false
	}

	command->endEncoding()

	r_resources.last_mipmapped_texture = cast(R_Texture_Handle)(sa.len(r_resources.textures))
}

r_get_bytes_per_pixel :: proc(format: MTL.PixelFormat) -> int {
	bytes_count := R_BYTES_PER_PIXEL[format]
	log.assertf(bytes_count != 0, "Invalid pixel format found %v", format)

	return bytes_count
}

r_get_mesh :: proc(handle: R_Mesh_Handle) -> (mesh: ^R_Mesh, ok: bool) #optional_ok #no_bounds_check {
	if r_is_resource_valid(handle) {
		return sa.get_ptr(&r_resources.meshes, cast(int)handle), true
	} else {
		return sa.get_ptr(&r_resources.meshes, 0), false
	}
}

r_get_texture :: proc(handle: R_Texture_Handle) -> (texture: ^R_Texture, ok: bool) #optional_ok #no_bounds_check {
	if r_is_resource_valid(handle) {
		return sa.get_ptr(&r_resources.textures, cast(int)handle), true
	} else {
		return sa.get_ptr(&r_resources.textures, 0), false
	}
}

r_get_material :: proc(handle: R_Material_Handle) -> (material: ^R_Material, ok: bool) #optional_ok #no_bounds_check {
	if r_is_resource_valid(handle) {
		return sa.get_ptr(&r_resources.materials, cast(int)handle), true
	} else {
		return sa.get_ptr(&r_resources.materials, 0), false
	}
}

r_register_resource :: proc{
	r_register_mesh,
	r_register_texture_from_bytes,
	r_register_texture_from_file,
	r_register_material,
}

r_register_texture :: proc{
	r_register_texture_from_bytes,
	r_register_texture_from_file,
}

r_is_resource_valid :: proc{
	r_is_mesh_valid,
	r_is_texture_valid,
	r_is_material_valid,
}

r_get_resource :: proc{
	r_get_mesh,
	r_get_texture,
	r_get_material,
}

r_gpu_handle_of :: proc{
	r_gpu_handle_of_mesh,
	r_gpu_handle_of_texture,
	r_gpu_handle_of_material,
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

