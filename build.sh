#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "==> Configuring..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo ""
echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release --parallel

echo ""
echo "==> Build complete!"
echo ""

AU_SRC="$BUILD_DIR/DarkSynth_artefacts/Release/AU/DarkSynth.component"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components/DarkSynth.component"

if [ -d "$AU_SRC" ]; then
    echo "==> Installing AU plugin to: $AU_DST"
    cp -r "$AU_SRC" "$AU_DST"
    echo ""
    echo "==> Validating AU plugin..."
    auval -v aumu Snth Manu || echo "(auval failed – plugin may still load in Logic)"
    echo ""
    echo "Done! Restart Logic Pro and scan for new plugins."
else
    echo "(AU artefact not found at expected path – check build output above)"
fi
