#!/bin/sh

set -e

if [[ "$1" = "metal" ]]; then
        cc _build.m -I../../include/ -O2 -g -o empty -framework Cocoa -framework Metal -framework QuartzCore -L../../build -lgpu -DMETAL4_RENDERER
elif [[ "$1" = "nogfx" ]]; then
        cc _build.m -I../../include/ -O2 -g -o empty -framework Cocoa -framework Metal -framework QuartzCore -L../../build -lgpu -DNOGFX_RENDERER
else
        echo "Usage: ./build.sh [metal|nogfx]"
fi
