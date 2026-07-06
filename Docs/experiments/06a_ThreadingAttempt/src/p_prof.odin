package main

p_init_prof :: #force_inline proc() -> bool {
	return #force_inline _p_init_prof()
}

p_prepare_thread_for_profiling :: #force_inline proc() {
	#force_inline _p_prepare_thread_for_profiling()
}

p_cleaup_thread_profiling_data :: #force_inline proc() {
	#force_inline _p_cleaup_thread_profiling_data()
}

p_fini_prof :: #force_inline proc() {
	#force_inline _p_fini_prof()
}

p_begin_profiling :: #force_inline proc() {
	#force_inline _p_begin_profiling()
}

p_end_profiling :: #force_inline proc() {
	#force_inline _p_end_profiling()
}

p_is_profiling :: #force_inline proc () -> bool {
	return #force_inline _p_is_profiling()
}


