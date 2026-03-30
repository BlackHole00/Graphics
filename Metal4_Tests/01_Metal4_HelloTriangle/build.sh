#!/bin/sh

set -e

odin build src -out:build/out -collection:shared=shared -debug -warnings-as-errors
codesign -s - -v -f --entitlements build/debug.plist build/out

