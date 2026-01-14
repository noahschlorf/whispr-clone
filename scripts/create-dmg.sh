#!/bin/bash
# Creates a distributable DMG for VoxType
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APP_NAME="VoxType"
APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
DMG_NAME="VoxType-Installer"
DMG_PATH="$BUILD_DIR/$DMG_NAME.dmg"
VOLUME_NAME="VoxType"

echo "Creating VoxType DMG installer..."

# Ensure app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
    echo "Error: App bundle not found at $APP_BUNDLE"
    echo "Please run create-app-bundle.sh first."
    exit 1
fi

# Clean up any existing DMG
rm -f "$DMG_PATH"

# Create a temporary directory for DMG contents
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo "Preparing DMG contents..."

# Copy app bundle to temp directory
cp -R "$APP_BUNDLE" "$TEMP_DIR/"

# Create Applications symlink
ln -s /Applications "$TEMP_DIR/Applications"

# Create a simple README
cat > "$TEMP_DIR/README.txt" << 'EOF'
VoxType - Voice to Text

Installation:
1. Drag VoxType.app to the Applications folder
2. Open VoxType from Applications
3. Grant Accessibility permissions when prompted:
   System Settings > Privacy & Security > Accessibility > VoxType
4. Hold Right Option key to record, release to transcribe

That's it! Your words will appear wherever your cursor is.

For help: https://github.com/noahschlorf/whispr-clone
EOF

# Calculate size needed for DMG (app size + 10MB buffer)
APP_SIZE=$(du -sm "$APP_BUNDLE" | cut -f1)
DMG_SIZE=$((APP_SIZE + 20))

echo "Creating DMG ($DMG_SIZE MB)..."

# Create DMG using hdiutil
# First create a read-write DMG
hdiutil create -size "${DMG_SIZE}m" \
    -fs HFS+ \
    -volname "$VOLUME_NAME" \
    -srcfolder "$TEMP_DIR" \
    -format UDRW \
    -ov \
    "$BUILD_DIR/temp.dmg"

# Mount the DMG to customize it
echo "Customizing DMG appearance..."
MOUNT_DIR=$(hdiutil attach "$BUILD_DIR/temp.dmg" -readwrite -noverify | grep "/Volumes/$VOLUME_NAME" | cut -f3)

if [ -n "$MOUNT_DIR" ]; then
    # Set icon positions using AppleScript
    osascript << EOF
    tell application "Finder"
        tell disk "$VOLUME_NAME"
            open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set bounds of container window to {100, 100, 600, 400}
            set theViewOptions to the icon view options of container window
            set arrangement of theViewOptions to not arranged
            set icon size of theViewOptions to 96
            set position of item "VoxType.app" of container window to {120, 150}
            set position of item "Applications" of container window to {380, 150}
            set position of item "README.txt" of container window to {250, 280}
            close
            open
            update without registering applications
            delay 1
            close
        end tell
    end tell
EOF

    # Unmount
    sync
    hdiutil detach "$MOUNT_DIR" -force
fi

# Convert to compressed read-only DMG
echo "Compressing DMG..."
hdiutil convert "$BUILD_DIR/temp.dmg" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -o "$DMG_PATH"

# Clean up temp DMG
rm -f "$BUILD_DIR/temp.dmg"

# Calculate final DMG size
DMG_FINAL_SIZE=$(du -h "$DMG_PATH" | cut -f1)

echo ""
echo "=== DMG created successfully! ==="
echo "Location: $DMG_PATH"
echo "Size: $DMG_FINAL_SIZE"
echo ""
echo "To test:"
echo "  open '$DMG_PATH'"
echo ""
echo "To distribute:"
echo "  Upload $DMG_PATH to GitHub Releases"
echo ""
