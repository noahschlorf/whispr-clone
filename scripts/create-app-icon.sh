#!/bin/bash
# Creates a simple app icon for VoxType using Python
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
ICONSET_DIR="$PROJECT_DIR/macos/AppIcon.iconset"
ICNS_FILE="$PROJECT_DIR/macos/AppIcon.icns"

echo "Creating VoxType app icon..."

# Check if we have pillow
if ! python3 -c "import PIL" 2>/dev/null; then
    echo "Installing Pillow for icon generation..."
    pip3 install --quiet pillow
fi

# Create iconset directory
rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

# Generate icon images using Python
python3 << 'PYTHON_SCRIPT'
from PIL import Image, ImageDraw
import os

def create_icon(size):
    # Create image with transparent background
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Calculate dimensions
    padding = int(size * 0.1)
    corner_radius = int(size * 0.2)

    # Draw rounded rectangle background (blue gradient simulation)
    # Since PIL doesn't support gradients easily, use solid color
    bg_color = (66, 133, 244, 255)  # Google blue
    draw.rounded_rectangle(
        [(padding, padding), (size - padding, size - padding)],
        radius=corner_radius,
        fill=bg_color
    )

    # Draw microphone icon (simplified)
    center_x = size // 2
    center_y = size // 2
    scale = size / 512

    # Microphone body
    mic_width = int(80 * scale)
    mic_height = int(140 * scale)
    mic_left = center_x - mic_width // 2
    mic_top = center_y - int(40 * scale)
    mic_radius = mic_width // 2

    # Draw mic body as rounded rectangle
    draw.rounded_rectangle(
        [(mic_left, mic_top), (mic_left + mic_width, mic_top + mic_height)],
        radius=mic_radius,
        fill=(255, 255, 255, 255)
    )

    # Draw stand (arc approximation with lines)
    arc_y = mic_top + mic_height - int(20 * scale)
    arc_radius = int(60 * scale)
    line_width = max(1, int(12 * scale))

    # Draw arc as thick line
    draw.arc(
        [(center_x - arc_radius, arc_y - arc_radius),
         (center_x + arc_radius, arc_y + arc_radius)],
        start=0, end=180,
        fill=(255, 255, 255, 255),
        width=line_width
    )

    # Stem
    stem_top = arc_y
    stem_bottom = arc_y + int(60 * scale)
    draw.line(
        [(center_x, stem_top), (center_x, stem_bottom)],
        fill=(255, 255, 255, 255),
        width=line_width
    )

    # Base
    base_width = int(80 * scale)
    draw.line(
        [(center_x - base_width // 2, stem_bottom),
         (center_x + base_width // 2, stem_bottom)],
        fill=(255, 255, 255, 255),
        width=line_width
    )

    return img

# Icon sizes for macOS
sizes = [
    (16, "icon_16x16.png"),
    (32, "icon_16x16@2x.png"),
    (32, "icon_32x32.png"),
    (64, "icon_32x32@2x.png"),
    (128, "icon_128x128.png"),
    (256, "icon_128x128@2x.png"),
    (256, "icon_256x256.png"),
    (512, "icon_256x256@2x.png"),
    (512, "icon_512x512.png"),
    (1024, "icon_512x512@2x.png")
]

iconset_dir = os.environ.get('ICONSET_DIR', 'macos/AppIcon.iconset')

for size, filename in sizes:
    img = create_icon(size)
    img.save(os.path.join(iconset_dir, filename))
    print(f"Created {filename}")

print("Icon images created!")
PYTHON_SCRIPT

# Set environment variable for Python script
export ICONSET_DIR="$ICONSET_DIR"

# Convert iconset to icns
if [ -d "$ICONSET_DIR" ] && [ "$(ls -A $ICONSET_DIR 2>/dev/null)" ]; then
    iconutil -c icns -o "$ICNS_FILE" "$ICONSET_DIR"
    echo "Created AppIcon.icns"
    rm -rf "$ICONSET_DIR"
else
    echo "Warning: Could not create icon set. Using default icon."
    exit 0
fi

echo "Done! Icon saved to: $ICNS_FILE"
