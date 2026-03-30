#!/bin/sh

set -e

if [ "$1" = "opt" ]; then
	odin build src -out:build/out -collection:shared=shared -o:aggressive -define:ENG_PROFILING=none -no-bounds-check -disable-assert -warnings-as-errors -show-timings
elif [ "$1" = "prof" ]; then
	odin build src -out:build/out -collection:shared=shared -define:ENG_PROFILING=spall -o:aggressive -no-bounds-check -disable-assert -warnings-as-errors -show-timings
else
	odin build src -out:build/out -collection:shared=shared -define:ENG_PROFILING=spall -debug -warnings-as-errors -show-timings
fi
codesign -s - -v -f --entitlements build/debug.plist build/out

