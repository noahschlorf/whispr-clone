#!/bin/bash
set -e

# Whispr Clone Build Script

echo "=== Building Whispr Clone ==="

# Create build directory
mkdir -p build
cd build

# Configure
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

echo ""
echo "=== Build Complete ==="
echo "Binary: build/whispr"
echo ""
echo "Next steps:"
echo "1. Download a whisper model:"
echo "   mkdir -p models"
echo "   curl -L -o models/ggml-base.en.bin \\"
echo "     https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"
echo ""
echo "2. Run the app:"
echo "   ./build/whispr"
