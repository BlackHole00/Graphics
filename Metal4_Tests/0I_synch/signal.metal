const char* signalKernelSource = R"msl(

#include <metal_stdlib>
#include <metal_atomic>

using namespace metal;

enum SignalOp {
	SIGNAL_OP_ATOMIC_SET,
	SIGNAL_OP_ATOMIC_MAX,
	SIGNAL_OP_ATOMIC_OR
};

struct SignalTask {
	device atomic_int*  signalSeq;
	device atomic_uint*    signalNumber;
	uint value;
};

constant uint signalOp [[function_constant(0)]];
kernel void signalValue(
	device SignalTask* task,
	uint threadId [[thread_index_in_threadgroup]]
) {
	if (threadId != 0) {
		return;
	}

	switch ((SignalOp)signalOp) {
		case SIGNAL_OP_ATOMIC_SET: {
			atomic_store_explicit(task->signalNumber, task->value, memory_order_relaxed);
			break;
		}
		case SIGNAL_OP_ATOMIC_OR: {
			atomic_fetch_or_explicit(task->signalNumber, task->value, memory_order_relaxed);
			break;
		}
		case SIGNAL_OP_ATOMIC_MAX: {
			atomic_fetch_max_explicit(task->signalNumber, task->value, memory_order_relaxed);
			break;
		}
	}
    
	atomic_thread_fence(mem_flags::mem_device, memory_order_seq_cst);

	atomic_fetch_add_explicit(task->signalSeq, 1, memory_order_relaxed);
}

)msl";

