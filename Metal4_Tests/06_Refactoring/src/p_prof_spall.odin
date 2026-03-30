#+private
package main

import "base:runtime"
import "core:prof/spall"
import "core:log"
import "core:sync"

_ :: runtime
_ :: spall
_ :: log
_ :: sync

when #config(ENG_PROFILING, "none") == "spall" {

	P_Prof :: struct {
		// atomic
		profiling:	bool,
		ctx:		spall.Context,
	}
	p_prof: P_Prof

	P_Prof_Tl :: struct {
		initialized:		bool,
		buffer:			spall.Buffer,
	}
	@(thread_local)
	p_prof_tl: P_Prof_Tl

	_p_init_prof :: proc() -> bool {
		spall_ok: bool
		p_prof.ctx, spall_ok = spall.context_create("prof.spall")

		if !spall_ok {
			log.error("Could not create a spall context.")
			return false
		}

		return true
	}

	_p_prepare_thread_for_profiling :: proc() {
		if p_prof_tl.initialized {
			return
		}

		// Will leak, but it's not a problem
		buffer_backing, err := make([]u8, spall.BUFFER_DEFAULT_SIZE * 128)
		if err != .None {
			log.warnf("Could not create a profiling buffer for thread %d.", sync.current_thread_id())
			return
		}

		p_prof_tl.buffer = spall.buffer_create(buffer_backing, u32(sync.current_thread_id()))
		p_prof_tl.initialized = true
	}

	_p_fini_prof :: proc() {
		spall.context_destroy(&p_prof.ctx)
	}

	_p_begin_profiling :: proc() {
		sync.atomic_store(&p_prof.profiling, true)
	}

	_p_end_profiling :: proc() {
		sync.atomic_store(&p_prof.profiling, false)
	}

	_p_is_profiling :: proc () -> bool {
		return sync.atomic_load(&p_prof.profiling)
	}

	@(instrumentation_enter)
	p_spall_enter :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
		if !sync.atomic_load(&p_prof.profiling) || !p_prof_tl.initialized {
			return
		}

		spall._buffer_begin(&p_prof.ctx, &p_prof_tl.buffer, "", "", loc)
	}

	@(instrumentation_exit)
	p_spall_exit :: proc "contextless" (proc_address, call_site_return_address: rawptr, loc: runtime.Source_Code_Location) {
		spall._buffer_end(&p_prof.ctx, &p_prof_tl.buffer)
	}

}
