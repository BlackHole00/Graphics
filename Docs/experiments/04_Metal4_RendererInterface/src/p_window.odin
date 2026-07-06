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

	p_init_inputs()

	target_view := glfw.GetCocoaView(p_window.handle)
	r_init(auto_cast target_view)

	g_init()
}

p_loop :: proc() {
	for !glfw.WindowShouldClose(p_window.handle) {
		if .Just_Pressed in p_get_keystate(glfw.KEY_ESCAPE) {
			glfw.SetWindowShouldClose(p_window.handle, true)
		}
		if .Just_Pressed in p_get_keystate(glfw.KEY_ENTER) && .Pressed in p_get_keystate(glfw.KEY_LEFT_SUPER) {
			window := glfw.GetCocoaWindow(p_window.handle)
			window->toggleFullScreen(nil)
		}

		p_platform_tick()
		g_tick()
		// r_render()

		p_update_inputs()
		glfw.PollEvents()

		free_all(context.temp_allocator)
	}
}

p_fini :: proc() {
	g_fini()
	r_fini()
	glfw.Terminate()
}

p_set_window_title :: proc(title: string) {
	ctitle := strings.clone_to_cstring(title, context.temp_allocator) or_else "Window"

	glfw.SetWindowTitle(p_window.handle, ctitle)
}

