#!/bin/bash
# VoxType Launcher
# Double-click to run VoxType

cd "$(dirname "$0")"

# Check if app bundle exists in Applications
if [ -d "/Applications/VoxType.app" ]; then
    open /Applications/VoxType.app
    osascript -e 'tell application "Terminal" to close front window' &
    exit 0
fi

# Check if local app bundle exists
if [ -d "build/VoxType.app" ]; then
    open build/VoxType.app
    osascript -e 'tell application "Terminal" to close front window' &
    exit 0
fi

# Fall back to running binary directly
if [ -f "build/voxtype" ]; then
    ./build/voxtype -q accurate &
    sleep 2
    osascript -e 'tell application "Terminal" to close front window' &
    exit 0
fi

# Nothing found - prompt to build
osascript -e 'display alert "VoxType Not Found" message "Please run ./build.sh first, or install VoxType.app to Applications."'
exit 1
