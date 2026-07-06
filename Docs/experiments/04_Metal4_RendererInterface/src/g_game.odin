package main

import "core:log"
import "core:math"
import "core:math/rand"
import la "core:math/linalg"
import "vendor:glfw"

G_Game :: struct {
	paused: bool,
}
g_game: G_Game

g_init :: proc() {
	g_init_actor_storage()

	player := G_Actor {
		enabled = true,

		position = { 0, 0, 0 },
		rotation = { 0, math.to_radians_f32(90), 0 },
		
		archetype = .Player,
		player = {},
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

	g_make_CUBES(128_000)
	// g_make_CUBES(10)
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
		cube.rotation.x = rand.float32_range(0, 2 * math.PI)
		cube.rotation.y = rand.float32_range(0, 2 * math.PI)
		cube.rotation.z = rand.float32_range(0, 2 * math.PI)
		cube.scale.xyz = rand.float32_range(0.5, 2)

		g_add_actor(&cube)
	}
	
}

g_fini :: proc() {
	g_fini_actor_storage()
}

g_tick :: proc() {
	c_scoped_autoreleasepool()

	if .Just_Pressed in p_get_keystate(glfw.KEY_P) {
		g_game.paused = !g_game.paused
	}

	g_tick_actors()

	if !r_prepare_frame() {
		return
	}
	g_render()
	r_end_frame()
}

g_render :: proc() {
	to_render := make([]R_Object_Instance, g_actor_count(), context.temp_allocator)
	to_render_count := 0

	camera: R_Camera
	did_find_camera := false

	iter := g_actor_iterator()
	for actor in g_iterate_actors(&iter) {
		if !actor.enabled {
			continue
		}

		if object, has_object := actor.render_object.(R_Object); has_object {
			to_render[to_render_count] = R_Object_Instance {
				mesh		= object.mesh,
				material	= object.material,
				model		= la.matrix4_translate(actor.position) *
							la.matrix4_rotate(actor.rotation.x, [3]f32{ 1, 0, 0 }) *
							la.matrix4_rotate(actor.rotation.y, [3]f32{ 0, 1, 0 }) *
							la.matrix4_rotate(actor.rotation.z, [3]f32{ 0, 0, 1 }) *
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
	
	r_draw(
		.Solid,
		camera,
		to_render[:to_render_count]
	)
} 

