# VoxType

**Voice-to-text that just works.** Hold a key, speak, release - your words appear wherever your cursor is.

100% local, 100% private. No internet required.

## Quick Install (macOS)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/noahschlorf/whispr-clone/master/install.sh)"
```

That's it! The script installs everything automatically.

## Manual Install

```bash
# 1. Install dependencies
brew install portaudio cmake

# 2. Clone
git clone --recursive https://github.com/noahschlorf/whispr-clone.git
cd whispr-clone

# 3. Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
cd ..

# 4. Download AI model (142MB)
mkdir -p models
curl -L -o models/ggml-base.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin

# 5. Run
./build/voxtype
```

## Grant Permissions (Required)

VoxType needs Accessibility permissions to detect your hotkey:

1. **System Settings** > **Privacy & Security** > **Accessibility**
2. Click **+** and add `voxtype` (in the build folder)
3. Toggle **ON**

## Usage

| Action | What happens |
|--------|-------------|
| **Hold Right Option** | Start recording |
| **Release** | Transcribe & paste |
| **Click menu bar icon** | See options & history |

### Menu Bar Features
- **Enable/Disable** - Toggle voice input on/off
- **Recent Transcriptions** - Click to copy previous text
- **Quality** - Switch between Fast/Balanced/Accurate/Best

## Options

```bash
./build/voxtype [options]

  -q, --quality MODE   fast, balanced, accurate, best (default: balanced)
  -t, --threads N      CPU threads (default: 4)
  --no-paste           Copy only, don't auto-paste
  -h, --help           Show all options
```

## Quality Modes

| Mode | Model | Size | Speed | Accuracy |
|------|-------|------|-------|----------|
| fast | tiny.en | 75MB | ~50ms | ~80% |
| **balanced** | base.en | 142MB | ~100ms | ~85% |
| accurate | small.en | 466MB | ~200ms | ~92% |
| best | medium.en | 1.5GB | ~500ms | ~95% |

Download other models:
```bash
# Faster but less accurate
curl -L -o models/ggml-tiny.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin

# More accurate but slower
curl -L -o models/ggml-small.en.bin \
  https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin
```

## How It Works

1. **PortAudio** captures microphone input
2. **whisper.cpp** runs OpenAI's Whisper model locally
3. **Metal GPU** accelerates inference on Apple Silicon
4. Text is copied to clipboard and pasted

All processing happens on your Mac. Nothing is sent to the cloud.

## Troubleshooting

**"Recording..." but nothing happens on release**
- Grant Accessibility permissions (see above)

**No menu bar icon**
- Make sure only one instance is running: `pkill voxtype`

**Model not found**
- Run from the project directory (where `models/` folder is)

## Requirements

- macOS 11+ (Big Sur or later)
- Apple Silicon (M1/M2/M3) or Intel Mac

## License

MIT
