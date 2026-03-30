package main

import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

R_Rendertarget :: struct {
	color_attachment:	^MTL.Texture,
	depth_attachment:	^MTL.Texture,
	size:			[2]uint,
}

r_rendetarget_create :: proc(rendertarget: ^R_Rendertarget, size: [2]uint, heap: ^MTL.Heap) {
	r_rendertarget_setup_for_size(rendertarget, size, heap)
}

r_rendertarget_resize :: proc(rendertarget: ^R_Rendertarget, size: [2]uint, heap: ^MTL.Heap) {
	rendertarget.color_attachment->release()
	rendertarget.depth_attachment->release()
	r_rendertarget_setup_for_size(rendertarget, size, heap)
}

r_rendertarget_setup_for_size :: proc(rendertarget: ^R_Rendertarget, size: [2]uint, heap: ^MTL.Heap) {

	color_descriptor := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(
		.RGBA8Unorm,
		cast(NS.UInteger)size.x,
		cast(NS.UInteger)size.y,
		false,
	)
	color_descriptor->setResourceOptions({})
	color_descriptor->setUsage(auto_cast MTL.TextureUsage.ShaderRead | MTL.TextureUsage.RenderTarget)

	depth_descriptor := MTL.TextureDescriptor.texture2DDescriptorWithPixelFormat(
		.Depth32Float,
		cast(NS.UInteger)size.x,
		cast(NS.UInteger)size.y,
		false,
	)
	depth_descriptor->setResourceOptions({})
	depth_descriptor->setUsage(.RenderTarget)
	
	rendertarget.color_attachment = heap->newTextureWithDescriptor(
		color_descriptor,
	)
	assert(rendertarget.color_attachment != nil)
	rendertarget.depth_attachment = heap->newTextureWithDescriptor(
		depth_descriptor,
	)
	assert(rendertarget.depth_attachment != nil)
}

