package main

import "core:log"
import "core:math"
import "core:math/rand"
import la "core:math/linalg"
import "vendor:glfw"

G_Game :: struct {
	// atomic
	should_close:	bool,
	paused:		bool,
}
g_game: G_Game

g_init :: proc() {
	g_init_actor_storage()

	player := G_Actor {
		enabled = true,

		position = { 0, 0, 0 },
		rotation = la.quaternion_from_euler_angles_f32(0, 0, 0, .XYZ),
		
		archetype = .Player,
		player = {
			movement_multiplier	= 0.01,
			h_look_sensibility	= 0.01,
			v_look_sensibility	= 0.0075,
		},
	}
	player_handle := g_add_actor(&player)

	camera := G_Actor {
		enabled = true,

		parent = player_handle,

		archetype = .Camera,
		camera	= {
			active	= true,
			fov	= 90.0,
			aspect	= 640/480,
			near	= 0.01,
			far	= 500.0,
		},
	}
	g_add_actor(&camera)

	// g_make_CUBES(128_000)
	g_make_CUBES(8192)
}

g_make_CUBES :: proc(n: int) {
	materials := [9]R_Material_Handle{}

	for &material, i in materials {
		material, _ = r_register_material({
			.Base_Color = cast(R_Texture_Handle)i,
			.Metallic = 0,
			.Roughness = 0,
			.Normal = 0,
			.Ambient_Occlusion = 0,
		})
	}

	for i in 0..<n {
		cube := G_Actor {
			enabled = true,

			scale = { 1, 1, 1 },

			archetype = .Dancing,
			render_object = R_Object {},
		}

		cube.position.x = rand.float32_range(-75, +75)
		cube.position.y = rand.float32_range(-75, +75)
		cube.position.z = rand.float32_range(-75, +75)
		cube.scale.xyz = rand.float32_range(0.5, 2)
		cube.rotation = la.quaternion_from_euler_angles(
			rand.float32_range(0, 2 * math.PI),
			rand.float32_range(0, 2 * math.PI),
			rand.float32_range(0, 2 * math.PI),
			.XYZ,
		)

		g_add_actor(&cube)
	}
	
}

g_fini :: proc() {
	g_fini_actor_storage()
}

g_tick :: proc() {
	// c_scoped_autoreleasepool()

	// if !r_prepare_frame() {
	// 	return
	// }

	// if .Just_Pressed in p_keystate_of(glfw.KEY_P) {
	// 	g_game.paused = !g_game.paused
	// }
	g_tick_actors()

	// g_render()
	// r_end_frame()
}

g_prepare_renderlist :: proc() -> (camera: R_Camera, renderlist: []R_Object_Instance) {
	renderlist = make([]R_Object_Instance, g_actor_count(), context.temp_allocator)
	to_render_count := 0

	did_find_camera := false

	iter := g_actor_iterator()
	for actor in g_iterate_actors(&iter) {
		if !actor.enabled {
			continue
		}

		if object, has_object := actor.render_object.(R_Object); has_object {
			renderlist[to_render_count] = R_Object_Instance {
				mesh		= object.mesh,
				material	= object.material,
				model		= la.matrix4_translate(actor.position) *
							la.to_matrix4(actor.rotation) *
							la.matrix4_scale(actor.scale),
			}
			to_render_count += 1
		}

		if actor.archetype == .Camera && actor.camera.active {
			g_camera_to_renderer_camera(actor, &camera)
			did_find_camera = true
		}
	}

	log.assert(did_find_camera, "Could not find an active camera actor. Aborting.")

	return camera, renderlist[:to_render_count]
}

g_render :: proc() {
	camera, renderlist := g_prepare_renderlist()
	
	r_draw(
		.Solid,
		camera,
		renderlist,
	)
} 

