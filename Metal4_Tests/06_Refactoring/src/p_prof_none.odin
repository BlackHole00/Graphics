#+private
package main

when #config(ENG_PROFILING, "none") == "none" {

	_p_init_prof :: proc() -> bool {
		return true
	}

	_p_prepare_thread_for_profiling :: proc() {}

	_p_fini_prof :: proc() {}

	_p_begin_profiling :: proc() {}

	_p_end_profiling :: proc() {}

	_p_is_profiling :: proc () -> bool {
		return false
	}

}

