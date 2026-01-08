# Whispr Clone

A low-latency voice-to-text application that transcribes speech and auto-pastes to your cursor location. Hold a hotkey to record, release to transcribe and paste.

## Features

- **Low Latency**: Uses local Whisper.cpp for fast transcription (~100-300ms)
- **Hold-to-Talk**: Hold Right Option/Alt to record, release to transcribe
- **Auto-Paste**: Text is automatically pasted where your cursor is
- **Menu Bar App**: Minimal UI with status indicator
- **Cross-Platform**: macOS and Linux support
- **GPU Acceleration**: Metal on macOS for faster inference

## Requirements

### macOS
- macOS 11+ (Big Sur or later)
- Homebrew
- Xcode Command Line Tools

### Linux
- libevdev (for keyboard input)
- X11 + XTest (for clipboard/paste)
- xclip or xsel

## Quick Start

```bash
# Clone the repo
git clone --recursive https://github.com/your-repo/whispr-clone.git
cd whispr-clone

# Build (macOS)
brew install portaudio cmake
./build.sh

# Download a Whisper model
mkdir -p models
curl -L -o models/ggml-base.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin

# Run
./build/whispr
```

## Usage

```
./build/whispr [options]

Options:
  -m, --model PATH    Path to whisper model (default: models/ggml-base.en.bin)
  -t, --threads N     Number of CPU threads (default: 4)
  -l, --language LANG Language code (default: en)
  -k, --keycode N     Hotkey keycode (platform-specific)
  --no-paste          Don't auto-paste, just copy to clipboard
  -h, --help          Show help
```

## Hotkey

- **macOS**: Right Option key (keycode 61)
- **Linux**: Right Alt key (keycode 108)

You can customize the hotkey with `-k <keycode>`.

## Permissions

### macOS
The app needs Accessibility permissions to capture keyboard events:
1. Go to System Preferences > Security & Privacy > Privacy > Accessibility
2. Add the `whispr` binary to the list

### Linux
Either run with sudo or add your user to the `input` group:
```bash
sudo usermod -a -G input $USER
# Log out and back in
```

## Models

Download Whisper models from HuggingFace:

| Model | Size | Speed | Quality |
|-------|------|-------|---------|
| tiny.en | 75 MB | Fastest | Good |
| base.en | 141 MB | Fast | Better |
| small.en | 466 MB | Medium | Great |
| medium.en | 1.5 GB | Slow | Excellent |

```bash
# Download different models
curl -L -o models/ggml-tiny.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin

curl -L -o models/ggml-small.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin
```

## Building from Source

```bash
# Install dependencies (macOS)
brew install portaudio cmake

# Install dependencies (Ubuntu/Debian)
sudo apt install portaudio19-dev cmake libevdev-dev libx11-dev libxtst-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## License

MIT
