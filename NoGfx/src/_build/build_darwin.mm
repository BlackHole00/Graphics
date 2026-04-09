#include <lib/common/language.h>

#ifndef CMN_LANGUAGE_OBJECTIVECPP
	#error NoGfx requires a compiler supporting Objective-C++ when targeting macOs.
#endif

#include <lib/common/page_posix.cpp>
#include <lib/common/pool.cpp>

#include <lib/lib.cpp>
#include <lib/layers.cpp>
#include <lib/layers_darwin.cpp>

#include <lib/metal4/layers.cpp>
#include <lib/metal4/context.mm>
#include <lib/metal4/device.mm>
#include <lib/metal4/allocation.mm>
#include <lib/metal4/validation.cpp>

