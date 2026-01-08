#!/bin/bash
# Whispr Manual Accuracy Test Suite
# Run this to test transcription accuracy with live speech

set -e

WHISPR_DIR="$(cd "$(dirname "$0")/.." && pwd)"
WHISPR_BIN="$WHISPR_DIR/build/whispr"

if [ ! -f "$WHISPR_BIN" ]; then
    echo "Error: whispr binary not found at $WHISPR_BIN"
    echo "Run: cd build && cmake .. && make"
    exit 1
fi

echo "==================================================="
echo "     Whispr Accuracy Test Suite"
echo "==================================================="
echo ""
echo "This script guides you through manual testing."
echo "For each test, hold the hotkey, speak the phrase,"
echo "then release to transcribe."
echo ""
echo "Whispr will run in the background. Check the terminal"
echo "for transcription output."
echo ""
echo "Press Ctrl+C to stop at any time."
echo "==================================================="
echo ""

# Start whispr in background
echo "Starting whispr..."
$WHISPR_BIN &
WHISPR_PID=$!
sleep 3

echo ""
echo "==================================================="
echo ""

run_test() {
    local test_name="$1"
    local phrase="$2"
    local notes="$3"

    echo "---"
    echo "Test: $test_name"
    echo "Say: \"$phrase\""
    if [ -n "$notes" ]; then
        echo "Notes: $notes"
    fi
    echo ""
    read -p "Press Enter when ready to speak..."
    echo "(Speak now, then release hotkey when done)"
    read -p "Press Enter after transcription appears..."
    echo ""
}

echo "=== Test 1: Clear Speech ==="
run_test "Clear Speech" \
    "Hello, this is a test of the transcription system." \
    "Speak clearly at normal volume"

echo "=== Test 2: Quiet Speech ==="
run_test "Quiet Speech" \
    "Can you hear me when I whisper quietly?" \
    "Speak quietly, almost whispering"

echo "=== Test 3: Loud Speech ==="
run_test "Loud Speech" \
    "Now I am speaking very loudly and clearly." \
    "Speak loudly, almost shouting"

echo "=== Test 4: Proper Nouns ==="
run_test "Proper Nouns" \
    "My name is Ralph Wiggum and I use Claude from Anthropic." \
    "Check if proper nouns are recognized correctly"

echo "=== Test 5: Technical Terms ==="
run_test "Technical Terms" \
    "I'm writing TypeScript code for the GitHub API using Python." \
    "Check if technical terms are recognized"

echo "=== Test 6: Rapid Start/Stop ==="
run_test "Rapid Start/Stop - Part 1" \
    "First short phrase." \
    "Say this quickly, then immediately do part 2"

run_test "Rapid Start/Stop - Part 2" \
    "Second short phrase." \
    "Check there's no duplication from part 1"

echo "=== Test 7: Long Sentence ==="
run_test "Long Sentence" \
    "This is a longer sentence that contains multiple clauses, including some complex vocabulary, proper punctuation, and should demonstrate how well the system handles extended speech with natural pauses." \
    "Speak naturally with pauses"

echo "=== Test 8: Numbers and Punctuation ==="
run_test "Numbers and Punctuation" \
    "The meeting is at 3:30 PM on January 15th, 2024. We have 42 items to discuss." \
    "Check numbers and dates are transcribed correctly"

echo "=== Test 9: Background Noise (if applicable) ==="
run_test "Background Noise" \
    "Testing with some background noise present." \
    "Try with music or ambient noise (optional)"

echo "=== Test 10: Filler Words ==="
run_test "Filler Words" \
    "Um, so like, I was thinking that, you know, we could maybe try this." \
    "Check if filler words are removed (um, like, you know)"

echo ""
echo "==================================================="
echo "     Tests Complete!"
echo "==================================================="
echo ""
echo "Review the transcriptions above and note:"
echo "  - Were words recognized correctly?"
echo "  - Were proper nouns capitalized?"
echo "  - Was punctuation added appropriately?"
echo "  - Were filler words removed?"
echo "  - Any duplication issues?"
echo ""
echo "Stopping whispr..."
kill $WHISPR_PID 2>/dev/null || true

echo ""
echo "Test session complete. Check output above for accuracy."
