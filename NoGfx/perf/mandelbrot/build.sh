#!/bin/sh

set -e

xcrun -sdk macosx metal -frecord-sources -gline-tables-only -c ./triangle.metal -o ./triangle.air
xcrun -sdk macosx metallib ./triangle.air -o ./triangle.metallib
xcrun -sdk macosx metal -frecord-sources -gline-tables-only -c ./triangle.vertex.metal -o ./triangle.vertex.air
xcrun -sdk macosx metallib ./triangle.vertex.air -o ./triangle.vertex.metallib
xcrun -sdk macosx metal -frecord-sources -gline-tables-only -c ./triangle.fragment.metal -o ./triangle.fragment.air
xcrun -sdk macosx metallib ./triangle.fragment.air -o ./triangle.fragment.metallib

if [[ "$1" = "metal" ]]; then
        cc _build.m -I../../include/ -O1 -g -o drawcalls -framework Cocoa -framework Metal -framework QuartzCore -L../../build -lgpu -DMETAL4_RENDERER
elif [[ "$1" = "nogfx" ]]; then
        cc _build.m -I../../include/ -O1 -g -o drawcalls -framework Cocoa -framework Metal -framework QuartzCore -L../../build -lgpu -DNOGFX_RENDERER
else
        echo "Usage: ./build.sh [metal|nogfx]"
fi
