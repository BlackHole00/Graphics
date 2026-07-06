#!/bin/sh

set -e

if [ "$2" = "shaders" ]; then
	./build_shaders.sh
fi
./build.sh $1

# MTL_DEBUG_LAYER=1 MTL_SHADER_VALIDATION=1 ./build/out
MTL_DEBUG_LAYER=1 ./build/out

