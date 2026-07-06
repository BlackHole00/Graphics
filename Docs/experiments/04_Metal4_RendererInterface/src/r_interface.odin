package main

import "core:time"
import "core:log"
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
	pvm:		matrix[4,4]f32,
	mesh:		MTL.GPUAddress,
	material:	MTL.GPUAddress,
}


r_draw :: proc(
	renderpass:	R_Renderpass,
	camera:		R_Camera,
	objects:	[]R_Object_Instance,
) {
	renderpass_desc := r_get_renderpassdescriptor_for_target_view()
	renderpass_encoder := r_renderer.frame_command_buffer->renderCommandEncoderWithDescriptor(renderpass_desc)

	proj := r_camera_proj_matrix(camera)
	view := r_camera_view_matrix(camera)

	frame_arena := r_current_arena()

	// if len(objects) > 0 {
	// 	renderpass_encoder->setRenderPipelineState(r_renderer.render_pipeline)
	// 	renderpass_encoder->setDepthStencilState(r_renderer.depth_stencil_state)
	// 	renderpass_encoder->setFrontFacingWinding(.CounterClockwise)
	// 	renderpass_encoder->setCullMode(.Back)

	// 	buffer, res := r_arena_new_buffer(frame_arena, size_of(R_Object) * len(objects))
	// 	assert(res == nil)

	// 	gpu_objects := cast([^]R_Object_Gpu_Repr)r_arena_buffer_contents(buffer)

	// 	r_renderer.argument_table->setAddress(r_arena_buffer_gpu_address(buffer), 0)
	// 	renderpass_encoder->setArgumentTable(r_renderer.argument_table, { .StageVertex, .StageFragment, })

	// 	for object, i in objects {
	// 		gpu_objects[i].pvm 	= proj * view * object.model
	// 		gpu_objects[i].mesh	= r_gpu_handle_of(object.mesh)
	// 		gpu_objects[i].material	= r_gpu_handle_of(object.material)

	// 		mesh := r_get_mesh(object.mesh)
	// 		renderpass_encoder->drawPrimitives(
	// 			.Triangle,
	// 			0,
	// 			cast(NS.UInteger)mesh.index_count,
	// 			1,
	// 			cast(NS.UInteger)i,
	// 		)
	// 	}
	// }
	if len(objects) > 0 {
		renderpass_encoder->setRenderPipelineState(r_renderer.render_pipeline)
		renderpass_encoder->setDepthStencilState(r_renderer.depth_stencil_state)
		renderpass_encoder->setFrontFacingWinding(.CounterClockwise)
		renderpass_encoder->setCullMode(.Back)

		begin := time.tick_now()

		// TODO: Improve this abomition
		mesh_map := make(map[R_Mesh_Handle][dynamic]^R_Object_Instance, context.temp_allocator)
		for &object in objects {
			arr, ok := &mesh_map[object.mesh]
			if !ok {
				arr_tmp := make([dynamic]^R_Object_Instance, context.temp_allocator)

				mesh_map[object.mesh] = arr_tmp
				arr = &mesh_map[object.mesh]
			}

			append(arr, &object)
		}
		mesh_map_creation_duration := time.tick_since(begin)
		// log.infof("Mesh map creation time: %f us", time.duration_microseconds(mesh_map_creation_duration))

		buffer, res := r_arena_new_buffer(frame_arena, size_of(R_Object) * len(objects))
		assert(res == nil)

		gpu_objects := cast([^]R_Object_Gpu_Repr)r_arena_buffer_contents(buffer)

		r_renderer.argument_table->setAddress(r_arena_buffer_gpu_address(buffer), 0)
		renderpass_encoder->setArgumentTable(r_renderer.argument_table, { .StageVertex, .StageFragment, })

		start_instance := 0
		i := 0
		for mesh, object_list in mesh_map {
			for object in object_list {
				gpu_objects[i].pvm 	= proj * view * object.model
				gpu_objects[i].mesh	= r_gpu_handle_of(object.mesh)
				gpu_objects[i].material	= r_gpu_handle_of(object.material)

				i += 1
			}

			mesh := r_get_mesh(mesh)
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

	renderpass_encoder->updateFence(r_renderer.free_to_present_fence, .StageFragment)
	renderpass_encoder->endEncoding()
}

