#!/bin/sh

set -e

xcrun -sdk macosx metal -o build/shader.metal.ir -c src/shader.metal -g -frecord-sources
xcrun -sdk macosx metal -o build/blit.metal.ir -c src/blit.metal -g -frecord-sources
xcrun -sdk macosx metallib -o res/shaders.metallib build/shader.metal.ir build/blit.metal.ir
