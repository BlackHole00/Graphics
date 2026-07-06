package main

import "core:math"
import la "core:math/linalg"

// The linalg package conventions apply (right-handed coordinate system):
//	- +x is right
//	- +y is up
//	- +z is forward

R_Camera :: struct {
	position: [3]f32,
	orientation: quaternion128,

	near: f32,
	far: f32,

	aspect: f32,
	fov: f32,
}


r_camera_view_matrix :: proc(camera: R_Camera) -> matrix[4,4]f32 {
	look_direction := la.mul(camera.orientation, [3]f32{ 0, 0, 1 })
	look_target := camera.position + look_direction

	view := la.matrix4_look_at(camera.position, look_target, [3]f32{ 0.0, 1.0, 0.0 })
	return view
}

r_camera_proj_matrix :: proc(camera: R_Camera) -> matrix[4,4]f32 {
	proj := la.matrix4_perspective_f32(math.to_radians(camera.fov), camera.aspect, camera.near, camera.far)
	return proj
}


r_update_camera_from_input :: proc(camera: ^R_Camera) {
	// left := la.cross()
}

