#!/bin/bash
# Run all automated tests

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "==================================================="
echo "     Whispr Automated Test Suite"
echo "==================================================="
echo ""

cd "$SCRIPT_DIR"

# Build audio processor test
echo "Building audio processor tests..."
g++ -std=c++17 -O2 \
    -I"$PROJECT_DIR/include" \
    -o test_audio_processor \
    test_audio_processor.cpp \
    "$PROJECT_DIR/src/audio_processor.cpp" \
    -lm 2>&1 || {
        echo "Failed to build audio processor tests"
        exit 1
    }

# Build text processor test
echo "Building text processor tests..."
g++ -std=c++17 -O2 \
    -I"$PROJECT_DIR/include" \
    -o test_text_processor \
    test_text_processor.cpp \
    "$PROJECT_DIR/src/text_processor.cpp" \
    2>&1 || {
        echo "Failed to build text processor tests"
        exit 1
    }

echo ""

# Run tests
echo "Running audio processor tests..."
./test_audio_processor || {
    echo "Audio processor tests FAILED"
    exit 1
}

echo ""
echo "Running text processor tests..."
./test_text_processor || {
    echo "Text processor tests FAILED"
    exit 1
}

echo ""
echo "==================================================="
echo "     All Automated Tests Passed!"
echo "==================================================="
echo ""
echo "To run manual accuracy tests, use: ./manual_test.sh"

# Clean up
rm -f test_audio_processor test_text_processor
