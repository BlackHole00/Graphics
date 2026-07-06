#!/bin/sh

set -e

./build_shaders.sh
./build.sh
# MTL_DEBUG_LAYER=1 MTL_SHADER_VALIDATION=1 ./build/out
MTL_DEBUG_LAYER=1 ./build/out
# ./build/out

