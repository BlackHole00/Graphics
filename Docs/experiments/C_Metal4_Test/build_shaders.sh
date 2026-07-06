#!/bin/sh

set -e

xcrun -sdk macosx metal -o build/shader.metal.ir -c src/shader.metal -g -frecord-sources -gline-tables-only
xcrun -sdk macosx metallib -o res/shaders.metallib build/shader.metal.ir
