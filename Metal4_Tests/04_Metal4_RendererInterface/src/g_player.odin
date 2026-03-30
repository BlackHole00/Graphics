package main

import "core:math"
import la "core:math/linalg"
import "vendor:glfw"

G_Player_Actor :: struct {}

// Minecraft creative mode style fly movement
// TODO: Ensure this gets run before the camera tick code, since the camera copies the player's position and rotation.
g_tick_player :: proc(player: ^G_Actor) {
	front := [3]f32 {
		math.cos(player.rotation.y),
		0.0,
		math.sin(player.rotation.y),
	}
	right := -la.cross(front, [3]f32{ 0, 1, 0 })

	movement := [3]f32{}
	rotation := [2]f32{}

	if .Pressed in p_get_keystate(glfw.KEY_W) {
		movement += front
	} else if .Pressed in p_get_keystate(glfw.KEY_S) {
		movement -= front
	}
	if .Pressed in p_get_keystate(glfw.KEY_D) {
		movement -= right
	} else if .Pressed in p_get_keystate(glfw.KEY_A) {
		movement += right
	}
	if .Pressed in p_get_keystate(glfw.KEY_SPACE) {
		movement += { 0,  1, 0 }
	} else if .Pressed in p_get_keystate(glfw.KEY_LEFT_SHIFT) {
		movement += { 0, -1, 0 }
	}

	if .Pressed in p_get_keystate(glfw.KEY_UP) {
		rotation.x += 1
	} else if .Pressed in p_get_keystate(glfw.KEY_DOWN) {
		rotation.x -= 1
	}
	if .Pressed in p_get_keystate(glfw.KEY_RIGHT) {
		rotation.y += 1
	} else if .Pressed in p_get_keystate(glfw.KEY_LEFT) {
		rotation.y -= 1
	}

	// TODO: Use deltatime
	// movement *= 1/60 * 10
	// rotation *= 1/60 * 10
	movement *= 0.05
	rotation *= 0.05

	player.position += movement
	player.rotation.xy += rotation

	if .Pressed in p_get_keystate(glfw.KEY_V) {
		player.position = { 100, 100, 100 }
		player.rotation = { math.to_radians_f32(-45), math.to_radians_f32(225), 0 }
	}
}

