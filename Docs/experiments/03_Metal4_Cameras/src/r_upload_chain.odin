package main

import "core:mem"
import MTL "shared:darwodin/Metal"

R_Upload_Chain :: struct {
	upload_buffer: ^MTL.Buffer,
}
r_upload_chain: R_Upload_Chain

r_init_upload_chain :: proc() {
	r_upload_chain.upload_buffer = r_renderer.device->newBufferWithLength(10 * mem.Megabyte, {})
}

r_do_queued_uploads :: proc() {
	
}

