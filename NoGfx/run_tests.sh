#!/usr/bin/env sh

set -e

./build.sh
c++ -o ./build/test -g -Wall -Wextra -Wpedantic -Isrc -Iinclude -Lbuild -lgpu -framework Foundation -framework Metal -std=c++11 ./test/main.cpp

./build/test

