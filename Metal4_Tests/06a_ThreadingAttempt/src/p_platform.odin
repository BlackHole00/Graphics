package main

import cc "shared:containers/cyclic_cell"

P_State :: struct {
	window:		P_Window,
	input:		P_Input,
	timings:	P_Timings,
}

P_Action :: struct {
	toggle_fullscreen:	bool,
	toggle_mouse_grab:	bool,
}

P_Platform :: struct {
	// The platform module produces states. The game module consumes states
	state:		cc.Cyclic_Cell(P_State),

	// The game module produces actions. The platform module consumes actions
	actions:	cc.Cyclic_Cell(P_Action),
}
p_platform: P_Platform


p_init_cells :: proc() {
	cc.create(&p_platform.state)
	cc.create(&p_platform.actions)
}

p_produce_current_state :: proc() {
	state := cc.producer_data(&p_platform.state)
	
	state.window = p_window
	state.input = p_input
	state.timings = p_timings

	cc.produce(&p_platform.state)
}

p_state :: proc() -> ^P_State {
	return cc.consumer_data(&p_platform.state)
}

p_advance_state :: proc() {
	cc.consume(&p_platform.state)
}

p_apply_actions :: proc() {
	actions := cc.consumer_data(&p_platform.actions)
	p_input_apply_actions(actions)

	cc.consume(&p_platform.actions)
}
