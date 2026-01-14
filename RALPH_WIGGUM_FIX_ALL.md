# Ralph Wiggum Plan: VoxType Complete Overhaul

## Overview

Comprehensive plan to fix all identified issues and make VoxType production-ready.

**54 issues identified across 11 categories:**
- 2 Critical, 4 High, 12 Medium, 36 Low priority items

---

## Phase 1: Critical Bug Fixes (Safety First)

### 1.1 Fix Null Pointer on HOME Environment Variable
**Location:** `src/vocabulary.cpp:12`
**Issue:** `std::getenv("HOME")` can return nullptr

```cpp
// BEFORE (unsafe):
const char* home = std::getenv("HOME");
std::string vocab_path = std::string(home) + "/.whispr/vocabulary.txt";

// AFTER (safe):
const char* home = std::getenv("HOME");
if (!home) {
    std::cerr << "Warning: HOME environment variable not set" << std::endl;
    return "";  // or use fallback path
}
std::string vocab_path = std::string(home) + "/.whispr/vocabulary.txt";
```

### 1.2 Fix Unsafe String-to-Integer Conversion
**Location:** `src/main.cpp:71, 77`
**Issue:** `std::atoi()` silently returns 0 on invalid input

```cpp
// BEFORE:
config.n_threads = std::atoi(argv[++i]);

// AFTER:
try {
    config.n_threads = std::stoi(argv[++i]);
} catch (const std::exception& e) {
    std::cerr << "Invalid thread count, using default" << std::endl;
    config.n_threads = 4;
}
```

### Verification Phase 1
- [ ] App doesn't crash when HOME is unset
- [ ] Invalid CLI arguments show helpful error
- [ ] Build passes with no warnings

---

## Phase 2: Thread Safety & Memory Management

### 2.1 Fix Global State Thread Safety
**Location:** `src/platform/macos/hotkey_macos.mm`, `tray_macos.mm`

Add proper synchronization:
```cpp
#include <mutex>

static std::mutex g_state_mutex;
static std::atomic<bool> g_enabled{true};

// Use lock_guard for all g_state access:
void update_state() {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    // ... modify state
}
```

### 2.2 Replace Raw new/delete with Smart Pointers
**Location:** `src/platform/macos/hotkey_macos.mm:27-39`

```cpp
// BEFORE:
static MacOSHotkeyState* g_state = nullptr;
g_state = new MacOSHotkeyState();
delete g_state;

// AFTER:
static std::unique_ptr<MacOSHotkeyState> g_state;
g_state = std::make_unique<MacOSHotkeyState>();
g_state.reset();  // automatic cleanup
```

### 2.3 Fix Memory Leak in Tray Icons
**Location:** `src/platform/macos/tray_macos.mm:108-109`

```objc
// Add cleanup in destroy_tray_icon():
void destroy_tray_icon() {
    if (g_icon_idle) { [g_icon_idle release]; g_icon_idle = nil; }
    if (g_icon_recording) { [g_icon_recording release]; g_icon_recording = nil; }
    if (g_icon_transcribing) { [g_icon_transcribing release]; g_icon_transcribing = nil; }
    if (g_icon_error) { [g_icon_error release]; g_icon_error = nil; }
    // ... existing cleanup
}
```

### 2.4 Synchronize History Vector Access
**Location:** `src/platform/macos/tray_macos.mm:13`

```cpp
static std::mutex g_history_mutex;
static std::vector<std::string> g_history;

void add_to_history(const std::string& text) {
    std::lock_guard<std::mutex> lock(g_history_mutex);
    g_history.push_back(text);
    if (g_history.size() > 10) g_history.erase(g_history.begin());
}
```

### Verification Phase 2
- [ ] No memory leaks (run with Instruments/valgrind)
- [ ] No data races (run with ThreadSanitizer)
- [ ] Clean shutdown with no crashes

---

## Phase 3: Error Handling Improvements

### 3.1 Add Error Handling to Audio Preprocessing
**Location:** `src/app.cpp:197-201`

```cpp
// BEFORE:
audio_processor_->process(audio_data);

// AFTER:
try {
    audio_processor_->process(audio_data);
} catch (const std::exception& e) {
    std::cerr << "Audio preprocessing failed: " << e.what() << std::endl;
    update_state(AppState::Error);
    return;
}
```

### 3.2 Improve Whisper Error Messages
**Location:** `src/transcriber.cpp:115-117`

```cpp
if (ret != 0) {
    result.error = "Whisper inference failed (code " + std::to_string(ret) + ")";
    if (ret == -1) result.error += ": Model not loaded";
    if (ret == -2) result.error += ": Audio too short";
    if (ret == -3) result.error += ": Out of memory";
    return result;
}
```

### 3.3 Add User Feedback for Silent Detection
**Location:** `src/app.cpp:225-230`

```cpp
if (audio_data.empty()) {
    std::cerr << "No speech detected in recording" << std::endl;
    update_state(AppState::Error);
    // Show notification to user
    show_notification("No Speech Detected",
                      "Try speaking louder or check microphone");
    return;
}
```

### Verification Phase 3
- [ ] All error paths show helpful messages
- [ ] User gets feedback when no speech detected
- [ ] Error states visible in menu bar

---

## Phase 4: Test Coverage Expansion

### 4.1 Fix Existing Test Bug
**Location:** `tests/test_text_processor.cpp:122`

```cpp
// BEFORE (operator precedence bug):
assert(result[0] == 'S' || result[0] == 'I' && "Should start with capital");

// AFTER:
assert((result[0] == 'S' || result[0] == 'I') && "Should start with capital");
```

### 4.2 Add Integration Tests
**Create:** `tests/test_integration.cpp`

```cpp
#include <cassert>
#include "app.hpp"
#include "transcriber.hpp"
#include "audio_processor.hpp"

void test_full_pipeline() {
    // Load test audio file
    auto audio = load_test_wav("tests/fixtures/hello_world.wav");

    // Process through pipeline
    AudioProcessor processor;
    processor.process(audio);

    Transcriber transcriber;
    transcriber.initialize("models/ggml-base.en.bin", 4);
    auto result = transcriber.transcribe(audio);

    assert(result.success);
    assert(result.text.find("hello") != std::string::npos ||
           result.text.find("Hello") != std::string::npos);
}

void test_error_recovery() {
    Transcriber transcriber;
    // Don't initialize - should fail gracefully
    std::vector<float> audio(16000, 0.0f);
    auto result = transcriber.transcribe(audio);
    assert(!result.success);
    assert(!result.error.empty());
}
```

### 4.3 Add Edge Case Tests
**Create:** `tests/test_edge_cases.cpp`

```cpp
void test_empty_audio() {
    Transcriber t;
    t.initialize("models/ggml-base.en.bin", 4);
    auto result = t.transcribe({});
    assert(!result.success);
}

void test_very_short_audio() {
    // Less than 100ms
    std::vector<float> audio(1000, 0.1f);  // ~62ms
    // Should handle gracefully
}

void test_very_long_audio() {
    // 60 seconds of audio
    std::vector<float> audio(16000 * 60, 0.0f);
    // Should handle or reject gracefully
}
```

### Verification Phase 4
- [ ] All existing tests still pass
- [ ] New integration tests pass
- [ ] Edge case tests pass
- [ ] Test coverage > 60%

---

## Phase 5: UX Improvements

### 5.1 Add Model Loading Progress
**Location:** `src/transcriber.cpp:14-35`

```cpp
bool Transcriber::initialize(const std::string& model_path, int n_threads,
                             std::function<void(int)> progress_cb) {
    if (progress_cb) progress_cb(0);

    // ... existing init code

    if (progress_cb) progress_cb(50);

    ctx_ = whisper_init_from_file_with_params(model_path.c_str(), cparams);

    if (progress_cb) progress_cb(100);
    return ctx_ != nullptr;
}
```

### 5.2 Show Confidence Score in Menu
**Location:** `src/platform/macos/tray_macos.mm`

```objc
void update_last_transcription(const std::string& text, float confidence) {
    NSString* display = [NSString stringWithFormat:@"%s (%.0f%%)",
                         text.c_str(), confidence * 100];
    // Add to menu
}
```

### 5.3 Add Sound Feedback Options
**Location:** `include/config.hpp`

```cpp
struct Config {
    // ... existing
    bool play_start_sound = true;   // Beep when recording starts
    bool play_end_sound = true;     // Beep when transcription done
    bool play_error_sound = true;   // Sound on error
};
```

### Verification Phase 5
- [ ] Loading progress shown for large models
- [ ] Confidence visible in menu
- [ ] Sound feedback works (when enabled)

---

## Phase 6: Performance Optimization

### 6.1 Optimize Text Processing
**Location:** `src/text_processor.cpp`

```cpp
// Pre-compile all regexes once at startup
class TextProcessor {
private:
    static const std::vector<std::pair<std::regex, std::string>> patterns_;
    static bool patterns_initialized_;

public:
    static void initialize_patterns();  // Call once at startup
};
```

### 6.2 Reduce Memory Allocations in VAD
**Location:** `src/audio_processor.cpp:222-257`

```cpp
// Reuse buffers instead of allocating new ones each time
class AudioProcessor {
private:
    std::vector<float> energy_buffer_;  // Reusable
    std::vector<float> smoothed_buffer_;

public:
    void process(std::vector<float>& audio) {
        // Resize and reuse instead of allocating
        energy_buffer_.resize(audio.size());
        // ...
    }
};
```

### 6.3 Add Processing Time Metrics
**Location:** `src/app.cpp`

```cpp
void App::process_recording() {
    auto start = std::chrono::high_resolution_clock::now();

    // ... processing

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Total processing time: " << ms.count() << "ms" << std::endl;
}
```

### Verification Phase 6
- [ ] Text processing < 1ms for typical input
- [ ] No new memory allocations during transcription
- [ ] Processing time logged and < 500ms for typical use

---

## Phase 7: Security Hardening

### 7.1 Validate User Input Paths
**Location:** `src/main.cpp`

```cpp
bool is_safe_path(const std::string& path) {
    // No path traversal
    if (path.find("..") != std::string::npos) return false;
    // Must exist
    if (!std::filesystem::exists(path)) return false;
    return true;
}

// Use before accepting model_dir:
if (!is_safe_path(argv[++i])) {
    std::cerr << "Invalid model path" << std::endl;
    return 1;
}
```

### 7.2 Sanitize Vocabulary Input
**Location:** `src/vocabulary.cpp`

```cpp
std::string sanitize_vocab_line(const std::string& line) {
    std::string clean;
    for (char c : line) {
        // Only allow alphanumeric, spaces, basic punctuation
        if (std::isalnum(c) || c == ' ' || c == '-' || c == '\'') {
            clean += c;
        }
    }
    return clean;
}
```

### Verification Phase 7
- [ ] Path traversal attempts rejected
- [ ] Malicious vocabulary content sanitized
- [ ] No shell injection possible

---

## Phase 8: Documentation Updates

### 8.1 Update README for Linux
Add Linux installation section:
```markdown
## Install (Linux)

```bash
# Ubuntu/Debian
sudo apt install portaudio19-dev cmake build-essential

# Fedora
sudo dnf install portaudio-devel cmake gcc-c++

# Build
./build.sh
```
```

### 8.2 Document All Config Options
Create `docs/CONFIGURATION.md`:
```markdown
# Configuration Options

## Audio Settings
- `sample_rate`: Must be 16000 (Whisper requirement)
- `silence_threshold`: Lower = more sensitive (0.005-0.05)
- `enhanced_vad`: Enable multi-segment speech detection

## Model Quality
- `fast`: tiny.en (75MB) - ~80% accuracy, 50ms
- `balanced`: base.en (142MB) - ~85% accuracy, 100ms
- `accurate`: small.en (466MB) - ~92% accuracy, 200ms
- `best`: medium.en (1.5GB) - ~95% accuracy, 500ms
```

### 8.3 Add API Documentation
Add Doxygen comments to all public methods:
```cpp
/**
 * @brief Transcribe audio data to text
 * @param audio PCM float samples at 16kHz mono
 * @return TranscriptionResult with text and confidence
 * @throws std::runtime_error if not initialized
 */
TranscriptionResult transcribe(const std::vector<float>& audio);
```

### Verification Phase 8
- [ ] README has Linux instructions
- [ ] All config options documented
- [ ] Key methods have doc comments

---

## Phase 9: Code Quality & Maintainability

### 9.1 Add Const Correctness
Review and fix const correctness throughout:
```cpp
// Parameters that shouldn't change:
TranscriptionResult transcribe(const std::vector<float>& audio) const;

// Member functions that don't modify state:
float calculate_confidence() const;
```

### 9.2 Replace Magic Numbers with Constants
**Location:** Various

```cpp
// In config.hpp:
namespace constants {
    constexpr int WHISPER_SAMPLE_RATE = 16000;
    constexpr int MIN_RECORDING_MS = 100;
    constexpr int MAX_RECORDING_MS = 30000;
    constexpr float DEFAULT_SILENCE_THRESHOLD = 0.01f;
    constexpr int HISTORY_MAX_SIZE = 10;
}
```

### 9.3 Extract Common Platform Code
Create `src/platform/common.hpp` for shared interfaces.

### Verification Phase 9
- [ ] No raw magic numbers in code
- [ ] Const correctness throughout
- [ ] Common patterns extracted

---

## Phase 10: Final Integration & Testing

### 10.1 Full Build Test
```bash
# Clean build
rm -rf build
./build.sh

# Should complete with no warnings
```

### 10.2 Run All Tests
```bash
cd tests
./run_tests.sh
```

### 10.3 Manual Testing Checklist
- [ ] App launches without errors
- [ ] Hotkey works (Right Option)
- [ ] Recording shows visual feedback
- [ ] Transcription appears in target app
- [ ] Menu bar shows history
- [ ] Quality switching works
- [ ] Error states show correctly
- [ ] Clean shutdown

### 10.4 Memory/Performance Validation
```bash
# macOS
leaks --atExit -- ./build/voxtype

# Check CPU usage during idle
# Should be < 1%
```

---

## Completion Criteria

All of the following must be true:

- [ ] All critical bugs fixed (Phase 1)
- [ ] Thread safety issues resolved (Phase 2)
- [ ] Error handling comprehensive (Phase 3)
- [ ] Test coverage > 60% (Phase 4)
- [ ] UX improvements implemented (Phase 5)
- [ ] No performance regressions (Phase 6)
- [ ] Security hardening complete (Phase 7)
- [ ] Documentation updated (Phase 8)
- [ ] Code quality improved (Phase 9)
- [ ] All tests pass (Phase 10)
- [ ] No memory leaks
- [ ] No compiler warnings

---

## Ralph Loop Execution Command

```bash
/ralph-loop "Fix and improve VoxType following RALPH_WIGGUM_FIX_ALL.md.

Execute phases 1-10 in order. For each phase:
1. Implement the fixes described
2. Build and verify no new warnings
3. Run tests to ensure no regressions
4. Commit changes with descriptive message

Priority order:
- Phase 1-2: CRITICAL - do first
- Phase 3-4: HIGH - error handling and tests
- Phase 5-7: MEDIUM - UX, perf, security
- Phase 8-9: LOW - docs and cleanup
- Phase 10: FINAL - integration testing

Output <promise>VOXTYPE_PRODUCTION_READY</promise> when:
- All 10 phases complete
- All tests pass
- No memory leaks
- No compiler warnings
- Manual testing checklist complete

If blocked on any phase:
- Document the specific blocker
- Attempt workaround
- If still blocked after 3 attempts, skip and note for manual review" --completion-promise "VOXTYPE_PRODUCTION_READY" --max-iterations 100
```

---

## Estimated Effort

| Phase | Effort | Priority |
|-------|--------|----------|
| Phase 1: Critical Bugs | 30 min | CRITICAL |
| Phase 2: Thread Safety | 1 hour | CRITICAL |
| Phase 3: Error Handling | 45 min | HIGH |
| Phase 4: Tests | 2 hours | HIGH |
| Phase 5: UX | 1.5 hours | MEDIUM |
| Phase 6: Performance | 1 hour | MEDIUM |
| Phase 7: Security | 45 min | MEDIUM |
| Phase 8: Documentation | 1 hour | LOW |
| Phase 9: Code Quality | 1 hour | LOW |
| Phase 10: Integration | 1 hour | FINAL |
| **TOTAL** | **~11 hours** | - |

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/vocabulary.cpp` | Null check, sanitization |
| `src/main.cpp` | Input validation, error handling |
| `src/app.cpp` | Error handling, metrics |
| `src/transcriber.cpp` | Error messages, progress |
| `src/audio_processor.cpp` | Buffer reuse, optimization |
| `src/text_processor.cpp` | Regex optimization |
| `src/platform/macos/hotkey_macos.mm` | Smart pointers, thread safety |
| `src/platform/macos/tray_macos.mm` | Memory leak fix, thread safety |
| `include/config.hpp` | Constants, new options |
| `tests/*.cpp` | New tests, bug fixes |
| `README.md` | Linux docs |
| `docs/CONFIGURATION.md` | NEW - config docs |

---

*Plan generated January 2026 based on comprehensive code analysis*
