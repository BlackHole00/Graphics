package main

import "core:time"

P_Timings :: struct {
	time_now:		time.Tick,
	last_frame_duration:	time.Duration,

	delta_time:		f32,
}
p_timings: P_Timings

p_init_timings :: proc() {
	p_timings.time_now = time.tick_now()
}

p_update_timings :: proc() {
	now := time.tick_now()

	p_timings.last_frame_duration = time.tick_diff(p_timings.time_now, now)
	p_timings.time_now = now

	p_timings.delta_time = cast(f32)(1 / time.duration_seconds(p_timings.last_frame_duration))
}

p_time_now :: proc() -> time.Tick {
	return p_timings.time_now
}

p_delta_time :: proc() -> f32 {
	return p_timings.delta_time
}

