#ifndef CMN_RESULT_H
#define CMN_RESULT_H

typedef enum CmnResult {
	CMN_SUCCESS		= 0,

	CMN_OUT_OF_MEMORY,
	CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED,
	CMN_OUT_OF_RESOURCE_SLOTS,
} CmnResult;

#define CMN_SET_RESULT(_res_ptr, _val) if (_res_ptr != nullptr) { *_res_ptr = _val; }

#endif // CMN_RESULT_H
