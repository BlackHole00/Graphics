package main

import "core:math/ease"
import "core:math/rand"
import "core:time"

G_Dancing_Actor :: struct {
	target:			[3]f32,
	start_position:		[3]f32,

	ms_travel_duration:	uint,
	start_time:		time.Tick,
}

g_tick_dancing :: proc(actor: ^G_Actor) {
	if actor.archetype != .Dancing || g_game.paused {
		return
	}

	if actor.dancing.start_time == {} || g_actor_is_near(actor^, actor.dancing.target) {
		actor.dancing.start_position = actor.position

		actor.dancing.target.x = rand.float32_range(-100, +100)
		actor.dancing.target.y = rand.float32_range(-100, +100)
		actor.dancing.target.z = rand.float32_range(-100, +100)

		actor.dancing.start_time = time.tick_now()
		actor.dancing.ms_travel_duration = rand.uint_range(2500, 20000)

		if render_object, ok := actor.render_object.(R_Object); ok {
			actor.render_object = R_Object {
				mesh = 0,
				material = (render_object.material + 1) % 7,
			}
		}
	}

	current_time := p_time_now()
	diff := time.duration_milliseconds(time.tick_diff(actor.dancing.start_time, current_time))

	if diff > cast(f64)actor.dancing.ms_travel_duration {
		diff = cast(f64)actor.dancing.ms_travel_duration
	}
	progress := diff / cast(f64)actor.dancing.ms_travel_duration
	
	easing_factor := ease.quartic_in_out(progress)

	travel_vector := actor.dancing.target - actor.dancing.start_position
	actor.position = travel_vector * cast(f32)easing_factor + actor.dancing.start_position
}

