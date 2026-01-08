#!/bin/bash
# Download whisper.cpp models for different quality levels

set -e

MODEL_DIR="${1:-models}"
BASE_URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main"

mkdir -p "$MODEL_DIR"

echo "Whisper Model Downloader"
echo "========================"
echo "Models will be saved to: $MODEL_DIR"
echo ""

download_model() {
    local name=$1
    local filename=$2
    local size=$3

    if [ -f "$MODEL_DIR/$filename" ]; then
        echo "[$name] Already exists: $filename"
        return
    fi

    echo "[$name] Downloading $filename ($size)..."
    curl -L --progress-bar -o "$MODEL_DIR/$filename" "$BASE_URL/$filename"
    echo "[$name] Done!"
}

echo "Available models:"
echo "  1) tiny.en   - 75MB  (Fast mode, ~80% accuracy)"
echo "  2) base.en   - 142MB (Balanced mode, ~85% accuracy)"
echo "  3) small.en  - 466MB (Accurate mode, ~92% accuracy)"
echo "  4) medium.en - 1.5GB (Best mode, ~95% accuracy)"
echo "  5) all       - Download all models"
echo ""

read -p "Select models to download (1-5, or comma-separated list like 1,2): " choice

download_tiny() { download_model "Fast" "ggml-tiny.en.bin" "75MB"; }
download_base() { download_model "Balanced" "ggml-base.en.bin" "142MB"; }
download_small() { download_model "Accurate" "ggml-small.en.bin" "466MB"; }
download_medium() { download_model "Best" "ggml-medium.en.bin" "1.5GB"; }

case "$choice" in
    1) download_tiny ;;
    2) download_base ;;
    3) download_small ;;
    4) download_medium ;;
    5)
        download_tiny
        download_base
        download_small
        download_medium
        ;;
    *)
        # Handle comma-separated list
        IFS=',' read -ra selections <<< "$choice"
        for sel in "${selections[@]}"; do
            sel=$(echo "$sel" | tr -d ' ')
            case "$sel" in
                1) download_tiny ;;
                2) download_base ;;
                3) download_small ;;
                4) download_medium ;;
            esac
        done
        ;;
esac

echo ""
echo "Download complete!"
echo ""
echo "Models in $MODEL_DIR:"
ls -lh "$MODEL_DIR"/*.bin 2>/dev/null || echo "  (no models found)"
