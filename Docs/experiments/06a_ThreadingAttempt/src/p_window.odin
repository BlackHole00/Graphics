package main

import "core:sync"
import "core:log"
import "core:thread"
import "core:strings"
import "vendor:glfw"

P_Window :: struct {
	handle: glfw.WindowHandle,
}
p_window: P_Window

p_init :: proc() {
	p_init_prof()

	p_prepare_thread_for_profiling()

	glfw.Init()

	glfw.WindowHint(glfw.CLIENT_API, glfw.NO_API)
	p_window.handle = glfw.CreateWindow(640, 480, "Metal", nil, nil)
	log.assert(p_window.handle != nil, "Could not create a GLFW window. Aborting.")

	p_init_cells()
	p_init_inputs()

	// target_view := glfw.GetCocoaView(p_window.handle)
	// r_init(auto_cast target_view)

	g_start_main_thread()

	p_init_timings()
	// g_init()
}

p_loop :: proc() {
	for !glfw.WindowShouldClose(p_window.handle) {
		if .Pressed in p_keystate_of(glfw.KEY_O) {
			p_begin_profiling()
		}

		if .Just_Pressed in p_keystate_of(glfw.KEY_ESCAPE) {
			glfw.SetWindowShouldClose(p_window.handle, true)
		}
		// if .Just_Pressed in p_keystate_of(glfw.KEY_ENTER) && .Pressed in p_keystate_of(glfw.KEY_LEFT_SUPER) {
		// 	window := glfw.GetCocoaWindow(p_window.handle)
		// 	window->toggleFullScreen(nil)
		// }

		p_produce_current_state()

		p_update_timings()
		// g_tick()

		p_apply_actions()
		p_update_inputs()
		glfw.PollEvents()

		free_all(context.temp_allocator)

		if .Pressed not_in p_keystate_of(glfw.KEY_O) && p_is_profiling() {
			p_end_profiling()
		}
	}
}

p_fini :: proc() {
	sync.atomic_store(&g_game.should_close, true)
	thread.join(g_thread)

	// g_fini()
	// r_fini()
	glfw.Terminate()
	p_cleaup_thread_profiling_data()
	p_fini_prof()
}

p_set_window_title :: proc(title: string) {
	ctitle := strings.clone_to_cstring(title, context.temp_allocator) or_else "Window"

	glfw.SetWindowTitle(p_window.handle, ctitle)
}

