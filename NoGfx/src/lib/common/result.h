#ifndef CMN_RESULT_H
#define CMN_RESULT_H

// Result codes returned by common library operations.
typedef enum CmnResult {
	// Operation completed successfully.
	CMN_SUCCESS		= 0,

	CMN_OUT_OF_MEMORY,
	// Virtual memory allocation failed.
	CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED,
	// No free slots are available in the target resource.
	CMN_OUT_OF_RESOURCE_SLOTS,

	// Provided parameters are invalid.
	CMN_INVALID_PARAMETERS,
	// Requested operation is not supported.
	CMN_UNSUPPORTED_OPERATION,
} CmnResult;

// Write a result code only when the destination pointer is valid.
#define CMN_SET_RESULT(_res_ptr, _val) if (_res_ptr != nullptr) { *_res_ptr = _val; }

#endif // CMN_RESULT_H
