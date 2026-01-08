#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APP_NAME="Whispr"
APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"

echo "Creating $APP_NAME.app bundle..."

# Create app bundle structure
rm -rf "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"

# Copy Info.plist
cp "$PROJECT_DIR/macos/Info.plist" "$APP_BUNDLE/Contents/"

# Copy executable with different name to avoid conflict with wrapper
cp "$BUILD_DIR/whispr" "$APP_BUNDLE/Contents/MacOS/whispr-bin"

# Copy models directory
if [ -d "$PROJECT_DIR/models" ]; then
    cp -r "$PROJECT_DIR/models" "$APP_BUNDLE/Contents/Resources/"
fi

# Create a simple wrapper script that sets the model path
cat > "$APP_BUNDLE/Contents/MacOS/Whispr" << 'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
RESOURCES="$DIR/../Resources"

# Use model from Resources if available, otherwise default location
if [ -f "$RESOURCES/models/ggml-base.en.bin" ]; then
    MODEL="$RESOURCES/models/ggml-base.en.bin"
elif [ -f "$HOME/.whispr/models/ggml-base.en.bin" ]; then
    MODEL="$HOME/.whispr/models/ggml-base.en.bin"
else
    MODEL="models/ggml-base.en.bin"
fi

exec "$DIR/whispr-bin" -m "$MODEL"
EOF
chmod +x "$APP_BUNDLE/Contents/MacOS/Whispr"

# Update Info.plist to use wrapper
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable Whispr" "$APP_BUNDLE/Contents/Info.plist"

echo ""
echo "=== App bundle created: $APP_BUNDLE ==="
echo ""
echo "To install:"
echo "  cp -r '$APP_BUNDLE' /Applications/"
echo ""
echo "To run:"
echo "  open '$APP_BUNDLE'"
echo ""
echo "Note: You'll need to grant Accessibility permissions on first run."
