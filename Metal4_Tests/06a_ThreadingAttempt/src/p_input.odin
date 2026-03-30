package main

import "vendor:glfw"
import cc "shared:containers/cyclic_cell"

P_Key_State :: enum {
	Released,
	Pressed,
	Just_Released,
	Just_Pressed,
}
P_Key_States :: bit_set[P_Key_State; byte]

P_Key :: enum {
	
}

P_Input :: struct {
	key_states: [glfw.KEY_LAST + 1]P_Key_States,
	mouse_button_states: [glfw.MOUSE_BUTTON_LAST + 1]P_Key_States,

	mouse_captured: bool,
	mouse_position: [2]f32,
	prev_frame_mouse_position: [2]f32,
}
p_input: P_Input

p_init_inputs :: proc() {
	glfw.SetKeyCallback(p_window.handle, p_glfw_key_callback)
	glfw.SetMouseButtonCallback(p_window.handle, p_glfw_mouse_button_callback)
	glfw.SetCursorPosCallback(p_window.handle, p_glfw_cursor_pos_callback)

	for &state in p_input.key_states {
		state = { .Released }
	}
	for &state in p_input.mouse_button_states {
		state = { .Released }
	}
}

p_keystate_of :: proc(#any_int glfw_key: int) -> P_Key_States {
	p_input := p_state().input
	return p_input.key_states[glfw_key]
}

p_mouse_button_state_of :: proc(#any_int glfw_button: int) -> P_Key_States {
	p_input := p_state().input
	return p_input.mouse_button_states[glfw_button]
}

p_mouse_movement_delta :: proc() -> [2]f32 {
	p_input := p_state().input
	return p_input.mouse_position - p_input.prev_frame_mouse_position
}

p_toggle_mouse_capture :: proc() {
	action := cc.producer_data(&p_platform.actions)

	action.toggle_mouse_grab = true
}

p_input_apply_actions :: proc(actions: ^P_Action) {
	if actions.toggle_mouse_grab {
		p_input.mouse_captured = !p_input.mouse_captured
		if p_input.mouse_captured {
			glfw.SetInputMode(p_window.handle, glfw.CURSOR, glfw.CURSOR_DISABLED)
		} else {
			glfw.SetInputMode(p_window.handle, glfw.CURSOR, glfw.CURSOR_NORMAL)
		}
	}
}

p_update_inputs :: proc() {
	for &state in p_input.key_states {
		if .Just_Pressed in state {
			state -= { .Just_Pressed }
		} else if .Just_Released in state {
			state -= { .Just_Released }
		}
	}

	for &state in p_input.mouse_button_states {
		if .Just_Pressed in state {
			state -= { .Just_Pressed }
		} else if .Just_Released in state {
			state -= { .Just_Released }
		}
	}

	p_input.prev_frame_mouse_position = p_input.mouse_position
}

p_glfw_key_callback :: proc "c" (_: glfw.WindowHandle, key, scancode, action, mods: i32) {
	if key < 0 || key > glfw.KEY_LAST {
		return
	}

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

p_glfw_mouse_button_callback :: proc "c" (_: glfw.WindowHandle, button, action, mods: i32) {
	if button < 0 || button > glfw.MOUSE_BUTTON_LAST {
		return
	}


	button_state := &p_input.mouse_button_states[button]

	switch {
	case action == glfw.PRESS && .Released not_in button_state:
		button_state^ = { .Pressed }

	case action == glfw.PRESS && .Released in button_state:
		button_state^ = { .Pressed, .Just_Pressed }

	case action == glfw.RELEASE && .Pressed not_in button_state:
		button_state^ = { .Released }

	case action == glfw.RELEASE && .Pressed in button_state:
		button_state^ = { .Released, .Just_Released }
	}
}

p_glfw_cursor_pos_callback :: proc "c" (_: glfw.WindowHandle, xpos, ypos: f64) {
	new_mouse_position := [2]f32{ cast(f32)xpos, cast(f32)ypos }

	p_input.prev_frame_mouse_position = p_input.mouse_position
	p_input.mouse_position = new_mouse_position
}

