#ifndef CMN_RESULT_H
#define CMN_RESULT_H

/**
	Result codes returned by common library operations.
*/
typedef enum CmnResult {
	/** The operation completed successfully. */
	CMN_SUCCESS		= 0,

	/** Memory allocation from the requested allocator failed. */
	CMN_OUT_OF_MEMORY,
	/** Virtual memory allocation failed. */
	CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED,
	/** No more free slots were available in the target resource. */
	CMN_OUT_OF_RESOURCE_SLOTS,

	/** The provided parameters are invalid. */
	CMN_INVALID_PARAMETERS,
	/** The requested operation is not supported. */
	CMN_UNSUPPORTED_OPERATION,
} CmnResult;

/**
	Sets a result code only when the destination pointer is valid.

	@param _res_ptr The destination result pointer.
	@param _val The result value to write.
*/
#define CMN_SET_RESULT(_res_ptr, _val) if (_res_ptr != nullptr) { *_res_ptr = _val; }

#endif // CMN_RESULT_H
