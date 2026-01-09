#!/bin/bash
# VoxType Installer - One-click voice-to-text for macOS
# Usage: curl -fsSL https://raw.githubusercontent.com/noahschlorf/whispr-clone/master/install.sh | bash

set -e

echo "=========================================="
echo "  VoxType Installer"
echo "  Voice-to-Text for macOS"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${RED}Error: VoxType currently only supports macOS${NC}"
    exit 1
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo -e "${YELLOW}Installing Homebrew...${NC}"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
brew install portaudio cmake 2>/dev/null || true

# Clone repo if not already in it
if [[ ! -f "CMakeLists.txt" ]]; then
    echo -e "${GREEN}Downloading VoxType...${NC}"
    git clone --recursive https://github.com/noahschlorf/whispr-clone.git ~/VoxType
    cd ~/VoxType
else
    echo -e "${GREEN}Building in current directory...${NC}"
fi

# Build
echo -e "${GREEN}Building VoxType...${NC}"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(sysctl -n hw.ncpu)
cd ..

# Copy Metal shader (required for GPU acceleration)
if [[ -f "external/whisper.cpp/ggml-metal.metal" ]]; then
    cp external/whisper.cpp/ggml-metal.metal .
fi

# Download models
mkdir -p models

# Download base model (required)
echo -e "${GREEN}Downloading Whisper base model (142MB)...${NC}"
if [[ ! -f "models/ggml-base.en.bin" ]]; then
    curl -L --progress-bar -o models/ggml-base.en.bin \
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
fi

# Download accurate model (recommended for better accuracy)
echo -e "${GREEN}Downloading Whisper accurate model (466MB)...${NC}"
if [[ ! -f "models/ggml-small.en.bin" ]]; then
    curl -L --progress-bar -o models/ggml-small.en.bin \
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin
fi

# Create launch script
echo -e "${GREEN}Creating launch script...${NC}"
cat > ~/VoxType/voxtype.command << 'LAUNCH'
#!/bin/bash
cd "$(dirname "$0")"
./build/voxtype -q accurate
LAUNCH
chmod +x ~/VoxType/voxtype.command

echo ""
echo "=========================================="
echo -e "${GREEN}  VoxType installed successfully!${NC}"
echo "=========================================="
echo ""
echo "To run VoxType (accurate mode - recommended):"
echo "  cd ~/VoxType && ./build/voxtype -q accurate"
echo ""
echo "Or double-click: ~/VoxType/voxtype.command"
echo ""
echo "For faster transcription (less accurate):"
echo "  cd ~/VoxType && ./build/voxtype -q balanced"
echo ""
echo -e "${YELLOW}IMPORTANT: Grant Accessibility permissions:${NC}"
echo "  1. Open System Settings > Privacy & Security > Accessibility"
echo "  2. Click '+' and add: ~/VoxType/build/voxtype"
echo "  3. Toggle ON"
echo ""
echo "Usage:"
echo "  - Hold RIGHT OPTION key to record"
echo "  - Release to transcribe and paste"
echo "  - Click menu bar mic icon for options"
echo ""
