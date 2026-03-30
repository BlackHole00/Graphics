package main

import "vendor:glfw"

// If a camera parent is set, then the camera will use its position and rotation to render the scene.
G_Camera_Actor :: struct {
	// True if this will be the camera used to render the scene.
	// TODO: Ensure that there will be only one active camera per scene
	active:	bool,

	fov:	f32,
	// TODO: The application logic should not have to think about the aspect rateo of the window
	aspect:	f32,
	
	near:	f32,
	far:	f32,
}

g_tick_camera :: proc(camera: ^G_Actor) {
	if camera.archetype != .Camera {
		return
	}

	parent, parent_ok := g_get_actor(camera.parent)
	if !parent_ok {
		return
	}
	
	width, height := glfw.GetWindowSize(p_window.handle)
	camera.camera.aspect = cast(f32)width / cast(f32)height

	camera.position = parent.position
	camera.rotation = parent.rotation
}

g_camera_to_renderer_camera :: proc(actor: ^G_Actor, camera: ^R_Camera) {
	if actor.archetype != .Camera {
		return
	}


	camera^ = {
		position	= actor.position,
		orientation	= actor.rotation,
		fov		= actor.camera.fov,
		aspect		= actor.camera.aspect,
		near		= actor.camera.near,
		far		= actor.camera.far,
	}
}

