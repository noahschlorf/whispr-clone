#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APP_NAME="VoxType"
APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"

echo "Creating $APP_NAME.app bundle..."

# Ensure the binary exists
if [ ! -f "$BUILD_DIR/voxtype" ]; then
    echo "Error: Binary not found at $BUILD_DIR/voxtype"
    echo "Please build the project first: ./build.sh"
    exit 1
fi

# Create app bundle structure
rm -rf "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"

# Copy Info.plist
cp "$PROJECT_DIR/macos/Info.plist" "$APP_BUNDLE/Contents/"

# Copy executable
cp "$BUILD_DIR/voxtype" "$APP_BUNDLE/Contents/MacOS/voxtype-bin"

# Copy Metal shader (required for GPU acceleration)
if [ -f "$PROJECT_DIR/ggml-metal.metal" ]; then
    cp "$PROJECT_DIR/ggml-metal.metal" "$APP_BUNDLE/Contents/Resources/"
fi

# Copy models directory (accurate model recommended)
if [ -d "$PROJECT_DIR/models" ]; then
    mkdir -p "$APP_BUNDLE/Contents/Resources/models"

    # Prioritize small.en (accurate) model
    if [ -f "$PROJECT_DIR/models/ggml-small.en.bin" ]; then
        echo "Bundling accurate model (small.en)..."
        cp "$PROJECT_DIR/models/ggml-small.en.bin" "$APP_BUNDLE/Contents/Resources/models/"
    fi

    # Also include base model as fallback
    if [ -f "$PROJECT_DIR/models/ggml-base.en.bin" ]; then
        echo "Bundling balanced model (base.en)..."
        cp "$PROJECT_DIR/models/ggml-base.en.bin" "$APP_BUNDLE/Contents/Resources/models/"
    fi
fi

# Copy vocabulary file if exists
if [ -f "$HOME/.whispr/vocabulary.txt" ]; then
    cp "$HOME/.whispr/vocabulary.txt" "$APP_BUNDLE/Contents/Resources/"
fi

# Create wrapper script that sets paths correctly
cat > "$APP_BUNDLE/Contents/MacOS/VoxType" << 'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
RESOURCES="$DIR/../Resources"

# Change to Resources directory so Metal shader can be found
cd "$RESOURCES"

# Determine best available model
if [ -f "$RESOURCES/models/ggml-small.en.bin" ]; then
    MODEL_DIR="$RESOURCES/models"
    QUALITY="accurate"
elif [ -f "$RESOURCES/models/ggml-base.en.bin" ]; then
    MODEL_DIR="$RESOURCES/models"
    QUALITY="balanced"
elif [ -d "$HOME/.voxtype/models" ]; then
    MODEL_DIR="$HOME/.voxtype/models"
    QUALITY="balanced"
else
    MODEL_DIR="models"
    QUALITY="balanced"
fi

# Run with proper quality setting
exec "$DIR/voxtype-bin" -m "$MODEL_DIR" -q "$QUALITY"
EOF
chmod +x "$APP_BUNDLE/Contents/MacOS/VoxType"

# Create app icon using iconutil (if we have an iconset)
if [ -d "$PROJECT_DIR/macos/AppIcon.iconset" ]; then
    iconutil -c icns -o "$APP_BUNDLE/Contents/Resources/AppIcon.icns" "$PROJECT_DIR/macos/AppIcon.iconset"
elif [ -f "$PROJECT_DIR/macos/AppIcon.icns" ]; then
    cp "$PROJECT_DIR/macos/AppIcon.icns" "$APP_BUNDLE/Contents/Resources/"
else
    # Create a simple placeholder icon using SF Symbols
    echo "Note: No app icon found. Using system default."
fi

# Calculate bundle size
BUNDLE_SIZE=$(du -sh "$APP_BUNDLE" | cut -f1)

echo ""
echo "=== App bundle created: $APP_BUNDLE ==="
echo "Size: $BUNDLE_SIZE"
echo ""
echo "To install:"
echo "  cp -r '$APP_BUNDLE' /Applications/"
echo ""
echo "To run:"
echo "  open '$APP_BUNDLE'"
echo ""
echo "Note: Grant Accessibility permissions on first run:"
echo "  System Settings > Privacy & Security > Accessibility > VoxType"
echo ""
