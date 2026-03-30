package main

import "core:log"
import "core:strings"
import "vendor:glfw"

P_Window :: struct {
	handle: glfw.WindowHandle,
}
p_window: P_Window

p_init :: proc() {
	glfw.Init()

	glfw.WindowHint(glfw.CLIENT_API, glfw.NO_API)
	p_window.handle = glfw.CreateWindow(640, 480, "Metal", nil, nil)
	log.assert(p_window.handle != nil, "Could not create a GLFW window. Aborting.")

	target_view := glfw.GetCocoaView(p_window.handle)
	r_init(auto_cast target_view)
}

p_loop :: proc() {
	for !glfw.WindowShouldClose(p_window.handle) {
		if glfw.GetKey(p_window.handle, glfw.KEY_ESCAPE) == glfw.PRESS {
			glfw.SetWindowShouldClose(p_window.handle, true)
		}

		r_render()

		glfw.PollEvents()

		free_all(context.temp_allocator)
	}
}

p_fini :: proc() {
	r_fini()
	glfw.Terminate()
}

p_set_window_title :: proc(title: string) {
	ctitle := strings.clone_to_cstring(title, context.temp_allocator) or_else "Window"

	glfw.SetWindowTitle(p_window.handle, ctitle)
}

