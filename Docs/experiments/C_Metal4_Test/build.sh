#!/bin/sh

clang __build.m -std=c11 -I./libs/rgfw/include -framework Cocoa -framework QuartzCore -framework Metal -g -o build/out
codesign -s - -v -f --entitlements build/debug.plist build/out

