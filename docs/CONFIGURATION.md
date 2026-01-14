# VoxType Configuration Guide

This document describes all configuration options available in VoxType.

## Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-q, --quality MODE` | Quality mode: fast, balanced, accurate, best | balanced |
| `-m, --model-dir DIR` | Directory containing whisper models | models |
| `-t, --threads N` | Number of CPU threads for inference | 4 |
| `-l, --language LANG` | Language code (e.g., en, es, fr) | en |
| `-k, --keycode N` | Hotkey keycode (platform-specific) | 61 (macOS), 108 (Linux) |
| `--no-paste` | Copy to clipboard only, don't auto-paste | false |
| `--no-preprocess` | Disable audio preprocessing | false |
| `-h, --help` | Show help message | - |

## Quality Modes

| Mode | Model | Size | Speed | Accuracy | Best For |
|------|-------|------|-------|----------|----------|
| fast | tiny.en | 75MB | ~50ms | ~80% | Quick notes |
| balanced | base.en | 142MB | ~100ms | ~85% | General use |
| accurate | small.en | 466MB | ~200ms | ~92% | Recommended |
| best | medium.en | 1.5GB | ~500ms | ~95% | Maximum quality |

## Audio Processing

VoxType includes several audio preprocessing stages to improve transcription accuracy:

### High-Pass Filter
- **Purpose:** Removes low-frequency rumble (HVAC, traffic, etc.)
- **Frequency:** 80 Hz cutoff
- **Enabled by default**

### Noise Gate
- **Purpose:** Suppresses quiet background noise
- **Threshold:** -34 dB
- **Attack:** 1 ms
- **Release:** 50 ms

### Automatic Gain Control (AGC)
- **Purpose:** Maintains consistent audio levels
- **Target RMS:** -16 dB
- **Max gain:** 20 dB
- **Min gain:** -20 dB

### Peak Normalization
- **Purpose:** Ensures audio peaks at consistent level
- **Target:** 90% of full scale

### Voice Activity Detection (VAD)
- **Purpose:** Trims silence from start/end of recordings
- **Threshold:** Adaptive based on noise floor
- **Padding:** 50 ms around speech segments

## Vocabulary Customization

Create a vocabulary file to improve recognition of names and technical terms:

**Location:** `~/.whispr/vocabulary.txt`

**Format:**
```
# Proper nouns - names of people, places, products
John Smith
Anthropic
Claude

# Technical terms - domain-specific vocabulary
API
TypeScript
whisper.cpp

# Common phrases - expressions you frequently use
Let me think about this
Could you please
```

The vocabulary file is automatically loaded on startup and used as context hints for the transcription model.

## Hotkey Codes

### macOS Key Codes
| Key | Code |
|-----|------|
| Right Option | 61 |
| Left Option | 58 |
| Right Command | 54 |
| Left Command | 55 |
| Caps Lock | 57 |
| F13 | 105 |

### Linux Key Codes (X11)
| Key | Code |
|-----|------|
| Right Alt | 108 |
| Left Alt | 64 |
| Right Control | 105 |
| Left Control | 37 |
| Caps Lock | 66 |

## Performance Tuning

### Thread Count
- **Apple Silicon (M1/M2/M3):** 4-6 threads recommended
- **Intel Mac:** Use number of performance cores
- **Linux:** `$(nproc)` or half of available cores

### GPU Acceleration
- **macOS:** Metal GPU is enabled by default
- **Linux:** CPU-only (CUDA support planned)

### Memory Usage
| Model | RAM Usage |
|-------|-----------|
| tiny.en | ~200 MB |
| base.en | ~500 MB |
| small.en | ~1 GB |
| medium.en | ~3 GB |

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `HOME` | Used for vocabulary file location | Required |
| `WHISPR_DEBUG` | Enable debug logging (0/1) | 0 |

## Troubleshooting Configuration

### Model Not Found
Ensure models are in the expected directory:
```bash
ls -la models/
# Should show ggml-*.bin files
```

### Audio Issues
Check microphone permissions and audio device:
```bash
# macOS
system_profiler SPAudioDataType

# Linux
arecord -l
```

### Permission Issues
- macOS: Grant Accessibility permission in System Settings
- Linux: Add user to audio group: `sudo usermod -aG audio $USER`
