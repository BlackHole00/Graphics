package main

import "core:math"
import la "core:math/linalg"

G_Actor_Handle :: struct {
	idx: u32,
	gen: u32,
}

G_Actor_Archetype :: enum {
	None,
	Camera,
	Player,
	Dancing,
}

G_Actor :: struct {
	handle:		G_Actor_Handle,
	enabled:	bool,

	position:	[3]f32,
	rotation:	[3]f32,
	scale:		[3]f32,

	parent:		G_Actor_Handle,
	// TODO: I'll add this later (maybe used to scenes). I'll use a circular siblings "linked list"
	// first_child:	G_Actor_Handle,
	// next_sibling:	G_Actor_Handle,

	render_object:	Maybe(R_Object),

	archetype:	G_Actor_Archetype,
	using data:	G_Actor_Archetype_Data,
}

G_Actor_Archetype_Data :: struct #raw_union {
	none:		struct{},
	camera:		G_Camera_Actor,
	player:		G_Player_Actor,
	dancing:	G_Dancing_Actor,
}

g_tick_actor :: proc(actor: ^G_Actor) {
	if !actor.enabled {
		return
	}

	switch actor.archetype {
	case .None:
	case .Player:
		g_tick_player(actor)
	case .Camera:
		g_tick_camera(actor)
	case .Dancing:
		g_tick_dancing(actor)
	}
}

g_actor_front_direction :: proc(actor: G_Actor) -> [3]f32 {
	front_direction := [3]f32 {
		math.cos(actor.rotation.y) * math.cos(actor.rotation.x),
		math.sin(actor.rotation.x),
		math.sin(actor.rotation.y) * math.cos(actor.rotation.x),
	}

	return front_direction
}

g_actor_is_near :: proc(actor: G_Actor, point: [3]f32, bias: f32 = 0.1) -> bool {
	return la.length(actor.position - point) < bias
}

