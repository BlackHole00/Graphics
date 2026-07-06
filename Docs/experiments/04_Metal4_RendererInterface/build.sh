#!/bin/sh

set -e

if [ "$1" = "opt" ]; then
	odin build src -out:build/out -collection:shared=shared -o:aggressive -no-bounds-check -disable-assert -warnings-as-errors -show-timings
else
	odin build src -out:build/out -collection:shared=shared -debug -warnings-as-errors -show-timings
fi
codesign -s - -v -f --entitlements build/debug.plist build/out

