#ifndef CMN_COMMONCOMMON_H
#define CMN_COMMONCOMMON_H

#include <stdint.h>
#include <stddef.h>

#include "architecture.h"
#include "platform.h"
#include "endianness.h"
#include "compiler.h"
#include "language.h"

#include "result.h"

#include "defer.h"

#define CMN_SET_NULLABLE(_var, _val) do { if (_var != nullptr) { *_var = _val; } } while(0)
#define CMN_COUNT_OF(_arr) (sizeof(_arr) / sizeof(*_arr))

#endif // CMN_COMMONCOMMON_H
