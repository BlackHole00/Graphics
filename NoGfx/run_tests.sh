#!/usr/bin/env sh

set -e

./build.sh
xcrun -sdk macosx metal -c ./test/shaders/compute.metal -o ./build/compute.air
xcrun -sdk macosx metallib ./build/compute.air -o ./build/compute.metallib
xcrun -sdk macosx metal -c ./test/shaders/render_vertex.metal -o ./build/render_vertex.air
xcrun -sdk macosx metallib ./build/render_vertex.air -o ./build/render_vertex.metallib
xcrun -sdk macosx metal -c ./test/shaders/render_fragment.metal -o ./build/render_fragment.air
xcrun -sdk macosx metallib ./build/render_fragment.air -o ./build/render_fragment.metallib
xcrun -sdk macosx metal -c ./test/shaders/meshlet.metal -o ./build/meshlet.air
xcrun -sdk macosx metallib ./build/meshlet.air -o ./build/meshlet.metallib
c++ -o ./build/test -g -Wall -Wextra -Wpedantic -Isrc -Iinclude -Lbuild -lgpu -framework Foundation -framework Metal -std=c++11 ./test/main.cpp
./build/test

./build/test

