package main

import "core:math"
import la "core:math/linalg"
import "vendor:glfw"

G_Player_Actor :: struct {
	movement_multiplier: f32,

	h_look_sensibility: f32,
	v_look_sensibility: f32,

	// This is required for the fps camera to avoid rotation wrap-arounds.
	yaw, pitch: f32,
}

// FPS style fly movement
// TODO: Ensure this gets run before the camera tick code, since the camera copies the player's position and rotation.
//	This currently updates in the correct order, but just because the player actor is created before the camera one.
g_tick_player :: proc(actor: ^G_Actor) {
	flat_front := [3]f32{ math.sin(actor.player.yaw), 0, math.cos(actor.player.yaw) }
	flat_right := la.cross(flat_front, [3]f32{ 0, 1, 0 })

	movement := [3]f32{}
	rotation := [2]f32{}

	if .Pressed in p_keystate_of(glfw.KEY_W) {
		movement += flat_front
	} else if .Pressed in p_keystate_of(glfw.KEY_S) {
		movement -= flat_front
	}
	if .Pressed in p_keystate_of(glfw.KEY_D) {
		movement += flat_right
	} else if .Pressed in p_keystate_of(glfw.KEY_A) {
		movement -= flat_right
	}
	if .Pressed in p_keystate_of(glfw.KEY_SPACE) {
		movement += { 0,  1, 0 }
	} else if .Pressed in p_keystate_of(glfw.KEY_LEFT_SHIFT) {
		movement += { 0, -1, 0 }
	}

	if p_input.mouse_captured {
		mouse_delta := p_mouse_movement_delta()
		rotation.x += mouse_delta.y
		rotation.y -= mouse_delta.x
	}

	movement *= actor.player.movement_multiplier * p_delta_time()

	rotation.x *= actor.player.v_look_sensibility
	rotation.y *= actor.player.h_look_sensibility

	actor.player.pitch	+= rotation.x
	actor.player.yaw	+= rotation.y
	actor.player.pitch	= clamp(actor.player.pitch, -math.PI * 0.445, math.PI * 0.445)

	q_yaw := la.quaternion_angle_axis(actor.player.yaw, [3]f32{ 0, 1, 0 })
	q_pitch := la.quaternion_angle_axis(actor.player.pitch, [3]f32{ 1, 0, 0 })

	actor.rotation = la.normalize(q_yaw * q_pitch)
	actor.position += movement

	if .Pressed in p_keystate_of(glfw.KEY_V) {
		actor.position = { 100, 100, 100 }
	}

	if .Just_Pressed in p_keystate_of(glfw.KEY_TAB) {
		p_toggle_mouse_capture()
	}
}

