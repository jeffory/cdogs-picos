#!/bin/bash
# Prepare C-Dogs data directory for PicOS SD card
# Usage: ./prepare_data.sh [SD_MOUNT_POINT]
#
# Copies required game data from the cdogs-sdl source tree to the
# SD card layout expected by the PicOS port.
#
# SD card layout:
#   /apps/cdogs/main.elf       (built by make)
#   /apps/cdogs/app.json       (app manifest)
#   /apps/cdogs/data/           (game data - this script creates this)
#     graphics/                  (sprites, font, etc.)
#     data/                      (JSON game definitions)
#     sounds/                    (audio files)
#     missions/                  (campaign files)
#     dogfights/                 (dogfight files)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
SD_ROOT="${1:-$SCRIPT_DIR}"
DATA_DIR="$SD_ROOT/data"

if [ ! -d "$SRC_DIR/graphics" ]; then
    echo "ERROR: Cannot find cdogs-sdl source at $SRC_DIR"
    echo "Expected: $SRC_DIR/graphics/ directory"
    exit 1
fi

echo "=== Preparing C-Dogs data for PicOS ==="
echo "Source:      $SRC_DIR"
echo "Destination: $DATA_DIR"
echo ""

# Create target directory
mkdir -p "$DATA_DIR"

# --- Graphics (sprites, font, etc.) ---
echo "Copying graphics..."
# Blender sources and art-pipeline scripts are ~19MB and are never opened
# at runtime; they also inflate directory entry counts during asset scans.
RUNTIME_EXCLUDES=(
    --exclude='*.blend' --exclude='*.blend1'
    --exclude='render.py' --exclude='make_spritesheet.sh'
    --exclude='README.md'
)
rsync -a --info=progress2 "${RUNTIME_EXCLUDES[@]}" \
    "$SRC_DIR/graphics/" "$DATA_DIR/graphics/"
echo "  Done: $(find "$DATA_DIR/graphics/" -name "*.png" | wc -l) PNG files"

# --- JSON data files ---
echo "Copying game data..."
mkdir -p "$DATA_DIR/data"
cp "$SRC_DIR/data/ammo.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/bullets.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/character_classes.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/guns.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/map_objects.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/particles.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/pickups.json" "$DATA_DIR/data/"
cp "$SRC_DIR/data/prefixes.txt" "$DATA_DIR/data/"
cp "$SRC_DIR/data/suffixes.txt" "$DATA_DIR/data/"
cp "$SRC_DIR/data/suffixnames.txt" "$DATA_DIR/data/"
echo "  Done: $(ls "$DATA_DIR/data/" | wc -l) files"

# --- Sounds ---
echo "Copying sounds..."
rsync -a --info=progress2 "${RUNTIME_EXCLUDES[@]}" \
    "$SRC_DIR/sounds/" "$DATA_DIR/sounds/"
echo "  Done: $(find "$DATA_DIR/sounds/" -type f | wc -l) sound files"

# --- Missions ---
echo "Copying missions..."
rsync -a --info=progress2 "${RUNTIME_EXCLUDES[@]}" \
    "$SRC_DIR/missions/" "$DATA_DIR/missions/"
echo "  Done: $(find "$DATA_DIR/missions/" -type f | wc -l) mission files"

# --- Dogfights ---
echo "Copying dogfights..."
rsync -a --info=progress2 "$SRC_DIR/dogfights/" "$DATA_DIR/dogfights/"
echo "  Done: $(find "$DATA_DIR/dogfights/" -type f | wc -l) dogfight files"

# --- Summary ---
echo ""
echo "=== Data preparation complete ==="
du -sh "$DATA_DIR"
echo ""
echo "To deploy to PicOS SD card:"
echo "  1. Copy main.elf to /apps/cdogs/ on SD card"
echo "  2. Copy app.json to /apps/cdogs/ on SD card"
echo "  3. Copy data/ directory to /apps/cdogs/data/ on SD card"
