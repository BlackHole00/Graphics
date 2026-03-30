package main

import "core:time"

P_Platform :: struct {
	tick_now:	time.Tick,
}
p_platform: P_Platform

p_platform_tick :: proc() {
	p_platform.tick_now = time.tick_now()
}

