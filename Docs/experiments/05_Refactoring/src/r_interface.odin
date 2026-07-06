package main

import NS "shared:darwodin/Foundation"
import MTL "shared:darwodin/Metal"

R_Renderpass :: enum {
	Solid
}

R_Object :: struct {
	mesh:		R_Mesh_Handle,
	material:	R_Material_Handle,
}
R_Object_Instance :: struct {
	model:		matrix[4,4]f32,
	mesh:		R_Mesh_Handle,
	material:	R_Material_Handle,
}
R_Object_Gpu_Repr :: struct #packed {
	model:		matrix[4,4]f32,
	mesh:		MTL.GPUAddress,
	material:	MTL.GPUAddress,
}

R_Camera_Data_Gpu_Repr :: struct #packed {
	projection:	matrix[4,4]f32,
	view:		matrix[4,4]f32,
}


R_Mesh_Map :: [][dynamic]^R_Object_Instance

r_prepare_mesh_map :: proc(renderlist: []R_Object_Instance) -> R_Mesh_Map {
	mesh_list_median_length := len(renderlist) / r_resources.meshes.len

	mesh_map := make([][dynamic]^R_Object_Instance, r_resources.meshes.len, context.temp_allocator)
	for &mesh_list in mesh_map {
		mesh_list = make(
			[dynamic]^R_Object_Instance,
			0,
			 mesh_list_median_length,
			context.temp_allocator,
		)
	}

	for &object in renderlist {
		arr := &mesh_map[object.mesh]
		append(arr, &object)
	}

	return mesh_map
}

r_draw_meshmap :: proc(
	renderpass_encoder:	^MTL.MTL4RenderCommandEncoder,
	camera:			R_Camera,
	mesh_map: 		R_Mesh_Map,
	total_object_count:	uint,
) {
	frame_arena := r_current_arena()

	proj := r_camera_proj_matrix(camera)
	view := r_camera_view_matrix(camera)

	camera_data_buffer, camera_buffer_res := r_arena_new_buffer(frame_arena, size_of(R_Camera_Data_Gpu_Repr))
	assert(camera_buffer_res == nil)
	camera_data_ptr := cast(^R_Camera_Data_Gpu_Repr)r_arena_buffer_contents(camera_data_buffer)
	camera_data_ptr.projection = proj
	camera_data_ptr.view = view

	object_instances_buffer, res := r_arena_new_buffer(frame_arena, size_of(R_Object) * total_object_count)
	assert(res == nil)

	gpu_objects := cast([^]R_Object_Gpu_Repr)r_arena_buffer_contents(object_instances_buffer)

	r_renderer.argument_table->setAddress(r_arena_buffer_gpu_address(camera_data_buffer), 0)
	r_renderer.argument_table->setAddress(r_arena_buffer_gpu_address(object_instances_buffer), 1)
	renderpass_encoder->setArgumentTable(r_renderer.argument_table, { .StageVertex, .StageFragment, })

	start_instance := 0
	i := 0
	for object_list, mesh_idx in mesh_map {
		mesh_handle := cast(R_Mesh_Handle)mesh_idx

		for object in object_list {
			gpu_objects[i].model 	= object.model
			gpu_objects[i].mesh	= r_gpu_handle_of(object.mesh)
			gpu_objects[i].material	= r_gpu_handle_of(object.material)

			i += 1
		}

		mesh := r_get_mesh(mesh_handle)
		renderpass_encoder->drawPrimitives(
			.Triangle,
			0,
			cast(NS.UInteger)mesh.index_count,
			cast(NS.UInteger)len(object_list),
			cast(NS.UInteger)start_instance,
		)

		start_instance = i
	}
}

r_draw :: proc(
	renderpass:	R_Renderpass,
	camera:		R_Camera,
	objects:	[]R_Object_Instance,
) {
	renderpass_desc := r_get_renderpassdescriptor_for_target_view()
	renderpass_encoder := r_renderer.frame_command_buffer->renderCommandEncoderWithDescriptor(renderpass_desc)

	if len(objects) > 0 {
		renderpass_encoder->setRenderPipelineState(r_renderer.render_pipeline)
		renderpass_encoder->setDepthStencilState(r_renderer.depth_stencil_state)
		renderpass_encoder->setFrontFacingWinding(.CounterClockwise)
		renderpass_encoder->setCullMode(.Back)

		mesh_map := r_prepare_mesh_map(objects)
		r_draw_meshmap(renderpass_encoder, camera, mesh_map, len(objects))
	}

	renderpass_encoder->updateFence(r_renderer.free_to_present_fence, .StageFragment)
	renderpass_encoder->endEncoding()
}

