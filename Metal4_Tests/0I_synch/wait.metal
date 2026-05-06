const char* waitKernelSource = R"msl(

#include <metal_stdlib>
#include <metal_atomic>

using namespace metal;

enum WaitOp : uint {
	WAIT_OP_NEVER,
	WAIT_OP_LESS,
	WAIT_OP_EQUAL,
	WAIT_OP_LESS_EQUAL,
	WAIT_OP_GREATER,
	WAIT_OP_NOT_EQUAL,
	WAIT_OP_GREATER_EQUAL,
	WAIT_OP_ALWAYS,
};

struct WaitTask {
	// An atomically increasing number. Every time it changes triggers a check for the thread to exit.
	device atomic_uint*	signalSeq;
	// The first number we need to wait before starting to checking for the thread to exit.
	uint			initialSignalSeq;
	// The location of the signal.
	device atomic_uint*	signalNumber;
	// The user wait number.
	uint			waitSignalNumber;
};

constant uint waitOp [[function_constant(0)]];
kernel void waitFor(
	device	WaitTask&	waitTask	[[buffer(0)]],
		uint		threadId	[[thread_position_in_grid]]
) {
	if (threadId != 0) {
		return;
	}

	uint waitingForSignalSeq = waitTask.initialSignalSeq + 1;

	for (;;) {
		uint currentSignalSeq;
		for (;;) {
			currentSignalSeq = atomic_load_explicit(waitTask.signalSeq, memory_order_relaxed);
			if (currentSignalSeq >= waitingForSignalSeq) {
				waitingForSignalSeq = currentSignalSeq + 1;
				break;
			}

			atomic_thread_fence(mem_flags::mem_device, memory_order_seq_cst);
		}

		uint currentSignalNumber = atomic_load_explicit(waitTask.signalNumber, memory_order_relaxed);

		switch ((WaitOp)waitOp) {
			case WAIT_OP_NEVER: {
				break;
			}
			case WAIT_OP_LESS: {
				if (currentSignalNumber < waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_EQUAL: {
				if (currentSignalNumber == waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_LESS_EQUAL: {
				if (currentSignalNumber <= waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_GREATER: {
				if (currentSignalNumber > waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_NOT_EQUAL: {
				if (currentSignalNumber != waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_GREATER_EQUAL: {
				if (currentSignalNumber >= waitTask.waitSignalNumber) {
					return;
				}
				break;
			}
			case WAIT_OP_ALWAYS: {
					return;
			}
		}
	}
}

)msl";

