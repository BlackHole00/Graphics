package main

import "vendor:glfw"

P_Key_State :: enum {
	Released,
	Pressed,
	Just_Released,
	Just_Pressed,
}
P_Key_States :: bit_set[P_Key_State; byte]

P_Input :: struct {
	key_states: [glfw.KEY_LAST]P_Key_States,
}
p_input: P_Input

p_init_inputs :: proc() {
	glfw.SetKeyCallback(p_window.handle, p_update_inputs_callback)

	for &state in p_input.key_states {
		state = { .Released }
	}
}

p_get_keystate :: proc(#any_int glfw_key: int) -> P_Key_States {
	return p_input.key_states[glfw_key]
}

p_update_inputs :: proc() {
	for &state in p_input.key_states {
		if .Just_Pressed in state {
			state -= { .Just_Pressed }
		} else if .Just_Released in state {
			state -= { .Just_Released }
		}
	}
}

p_update_inputs_callback :: proc "c" (_: glfw.WindowHandle, key, scancode, action, mods: i32) {
	key_state := &p_input.key_states[key]

	switch {
	case action == glfw.PRESS && .Released not_in key_state:
		key_state^ = { .Pressed }

	case action == glfw.PRESS && .Released in key_state:
		key_state^ = { .Pressed, .Just_Pressed }

	case action == glfw.RELEASE && .Pressed not_in key_state:
		key_state^ = { .Released }

	case action == glfw.RELEASE && .Pressed in key_state:
		key_state^ = { .Released, .Just_Released }
	}
}

