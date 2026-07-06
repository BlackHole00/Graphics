package main

import "core:sync"
import "core:thread"
import "vendor:glfw"
import cc "shared:containers/cyclic_cell"

g_thread: ^thread.Thread

g_main_thread :: proc() {
	p_prepare_thread_for_profiling()
	defer p_cleaup_thread_profiling_data()

	// g_init()
	// defer g_fini()

	for !sync.atomic_load(&g_game.should_close) {
		p_advance_state()
		// g_tick()
		if .Just_Pressed in p_keystate_of(glfw.KEY_TAB) {
			p_toggle_mouse_capture()
		}

		cc.produce(&p_platform.actions)
	}

	// for !sync.atomic_load(&g_game.should_close) {
	// 	g_tick()
	// }

	// g_fini()
}

g_start_main_thread :: proc() -> ^thread.Thread {
	g_thread = thread.create_and_start(g_main_thread, context, .High, false)

	return g_thread
}

