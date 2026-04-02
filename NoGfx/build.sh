#!/usr/bin/env sh

set -e

c++ -c -o build/gpu.o -fno-objc-arc -Wall -Wextra -Wpedantic -Iinclude -Isrc -std=c++11 -g -xobjective-c++ src/_build/build.cpp

ar rcs build/libgpu.a build/gpu.o

