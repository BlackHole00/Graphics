package main

import CF "shared:darwodin/CoreFoundation"
import NS "shared:darwodin/Foundation"

@(deferred_out=c_scoped_autoreleasepool_pop)
c_scoped_autoreleasepool :: proc() -> ^NS.AutoreleasePool {
	return NS.AutoreleasePool.new()
}

c_scoped_autoreleasepool_pop :: proc(pool: ^NS.AutoreleasePool) {
	pool->release()
}

c_AT :: proc(str: cstring) -> ^NS.String {
	foundation_str := CF.StringMakeConstantString(str)
	return cast(^NS.String)foundation_str
}

