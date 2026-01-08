# Ralph Wiggum Plan: Whisper Accuracy Improvements v2

## Overview

Comprehensive plan to significantly improve transcription accuracy in whispr-clone based on industry best practices, GitHub research, and OpenAI recommendations.

## Current State

- **Model**: base.en (147MB) - good balance but not optimal accuracy
- **Beam size**: 5 (good)
- **Best of**: 5 (good)
- **VAD**: Basic RMS-based silence trimming
- **Audio preprocessing**: High-pass filter, noise gate, normalization
- **Known issues**: Words not picked up perfectly, accuracy varies

## Target State

- 90%+ transcription accuracy for clear speech
- Robust handling of background noise
- Better recognition of proper nouns and technical terms
- Comprehensive testing framework to validate improvements

---

## Phase 1: Upgrade to Better Model (Optional - User Choice)

### Background
The base.en model (147MB) has ~85% accuracy. The small.en model (466MB) achieves ~92% accuracy.

### Tasks
1. Check if user wants to upgrade model (small.en recommended)
2. Download small.en model if chosen:
   ```bash
   curl -L -o models/ggml-small.en.bin \
     https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin
   ```
3. Update config default to use small.en if available, fallback to base.en

### Verification
- [ ] Model downloads successfully (~466MB)
- [ ] App starts with new model
- [ ] Transcription still works

### User Decision Required
- [ ] Ask user: "Download small.en model for better accuracy? (466MB, ~92% accuracy vs 85%)"

---

## Phase 2: Optimize Whisper Parameters

### Background
OpenAI's optimal settings differ from whisper.cpp defaults. Key improvements:
- Lower no_speech_threshold for better silence detection
- Compression ratio threshold to prevent repetition
- Temperature fallback for difficult audio

### Tasks
1. Update `include/config.hpp` with new TranscriptionProfile:
   ```cpp
   // Optimized for accuracy based on OpenAI recommendations
   inline const TranscriptionProfile PROFILE_OPTIMIZED = {
       .best_of = 5,
       .beam_size = 5,
       .entropy_thold = 2.4f,
       .no_speech_thold = 0.3f,  // Lower = more sensitive (was 0.5)
       .temperature = 0.0f,
       .name = "Optimized"
   };
   ```

2. Update `src/transcriber.cpp` to add these parameters:
   ```cpp
   wparams.compression_ratio_threshold = 2.4f;  // Detect repetition
   wparams.logprob_threshold = -1.0f;           // Confidence measure
   wparams.temperature_inc = 0.2f;              // Fallback temperature
   wparams.max_initial_ts = 1.0f;               // First timestamp limit
   ```

3. Enable word-level timestamps for debugging:
   ```cpp
   wparams.token_timestamps = true;
   ```

### Verification
- [ ] Code compiles without errors
- [ ] Run test: "Hello, this is a test" transcribes correctly
- [ ] Run test: Whispered speech still works
- [ ] Run test: Speech with background noise works

---

## Phase 3: Implement Silero VAD Integration

### Background
Silero VAD is ultra-fast (~1ms per 30s chunk) and significantly improves accuracy by:
- Filtering out silence/noise before transcription
- Reducing hallucinations
- Enabling precise speech boundaries

### Tasks
1. Download Silero VAD ONNX model:
   ```bash
   curl -L -o models/silero_vad.onnx \
     https://github.com/snakers4/silero-vad/raw/master/files/silero_vad.onnx
   ```

2. Create `include/vad.hpp`:
   ```cpp
   #pragma once
   #include <vector>

   namespace whispr {

   struct SpeechSegment {
       int start_sample;
       int end_sample;
       float confidence;
   };

   class VoiceActivityDetector {
   public:
       VoiceActivityDetector();
       ~VoiceActivityDetector();

       bool initialize(const std::string& model_path);

       // Detect speech segments in audio
       std::vector<SpeechSegment> detect(const std::vector<float>& audio,
                                          int sample_rate = 16000);

       // Get only speech portions of audio
       std::vector<float> extract_speech(const std::vector<float>& audio,
                                          int sample_rate = 16000,
                                          int padding_ms = 100);

       void set_threshold(float threshold) { threshold_ = threshold; }
       void set_min_speech_ms(int ms) { min_speech_ms_ = ms; }
       void set_padding_ms(int ms) { padding_ms_ = ms; }

   private:
       float threshold_ = 0.5f;
       int min_speech_ms_ = 250;
       int padding_ms_ = 100;
       // ONNX runtime handle
       void* session_ = nullptr;
   };

   } // namespace whispr
   ```

3. Create `src/vad.cpp` with ONNX runtime or simpler energy-based VAD

4. **Alternative**: Use whisper.cpp built-in VAD if available in newer versions

### Verification
- [ ] VAD model loads successfully
- [ ] VAD correctly identifies speech vs silence
- [ ] Test: Recording with 2s silence at start → silence trimmed
- [ ] Test: Recording with background music → speech extracted
- [ ] No false positives (silence transcribed as speech)

---

## Phase 4: Enhanced Audio Preprocessing

### Background
Audio quality significantly impacts accuracy. Enhancements:
- Better noise reduction (without distorting speech)
- Automatic gain control
- Echo cancellation (if needed)

### Tasks
1. Update `AudioProcessor` with new features:

   ```cpp
   struct AudioProcessorConfig {
       // Existing
       bool enable_highpass = true;
       float highpass_freq = 80.0f;
       bool enable_noise_gate = true;
       float noise_gate_threshold = 0.02f;
       bool enable_normalization = true;
       float target_peak = 0.9f;

       // NEW: Enhanced preprocessing
       bool enable_agc = true;              // Automatic gain control
       float agc_target_level = 0.7f;       // Target RMS level
       bool enable_declick = true;          // Remove clicks/pops
       bool enable_spectral_gate = false;   // Advanced noise reduction
   };
   ```

2. Implement Automatic Gain Control (AGC):
   ```cpp
   void AudioProcessor::apply_agc(std::vector<float>& audio) {
       // Calculate current RMS
       float rms = calculate_rms(audio);
       if (rms < 0.001f) return;  // Too quiet

       // Apply gain to reach target
       float gain = config_.agc_target_level / rms;
       gain = std::min(gain, 10.0f);  // Max 20dB boost

       for (float& sample : audio) {
           sample *= gain;
           sample = std::clamp(sample, -1.0f, 1.0f);
       }
   }
   ```

3. Implement click/pop removal (optional)

### Verification
- [ ] AGC boosts quiet speech to consistent level
- [ ] Loud speech not clipped
- [ ] Test: Whispered speech → normalized and transcribed correctly
- [ ] Test: Shouted speech → not distorted

---

## Phase 5: Improved Initial Prompt System

### Background
The initial prompt helps Whisper recognize domain-specific vocabulary.
Limitations: 224 tokens max, only affects first 30 seconds.

### Tasks
1. Create configurable vocabulary system:

   ```cpp
   // In config.hpp
   struct VocabularyConfig {
       std::vector<std::string> proper_nouns;    // Names, places
       std::vector<std::string> technical_terms; // Domain terms
       std::vector<std::string> common_phrases;  // Frequently used phrases
   };
   ```

2. Add user-editable vocabulary file `~/.whispr/vocabulary.txt`:
   ```
   # Proper nouns
   Ralph Wiggum
   Claude
   Anthropic

   # Technical terms
   API
   GitHub
   TypeScript

   # Common phrases
   Let me think about this
   ```

3. Build optimized initial prompt from vocabulary:
   ```cpp
   std::string build_initial_prompt(const VocabularyConfig& vocab) {
       std::string prompt = "Transcription context: ";
       for (const auto& noun : vocab.proper_nouns) {
           prompt += noun + ", ";
       }
       // Trim to 224 tokens
       return truncate_to_tokens(prompt, 200);
   }
   ```

### Verification
- [ ] Vocabulary file loads correctly
- [ ] Custom vocabulary improves recognition
- [ ] Test: Say "Ralph Wiggum" → recognized correctly
- [ ] Test: Say technical terms → recognized correctly

---

## Phase 6: Comprehensive Testing Framework

### Tasks
1. Create `tests/test_accuracy.cpp`:

   ```cpp
   #include <gtest/gtest.h>
   #include "transcriber.hpp"
   #include "audio_capture.hpp"

   class AccuracyTest : public ::testing::Test {
   protected:
       Transcriber transcriber;

       void SetUp() override {
           transcriber.initialize("models/ggml-base.en.bin", 4);
       }

       // Load test audio file
       std::vector<float> load_wav(const std::string& path);

       // Calculate Word Error Rate
       float calculate_wer(const std::string& reference,
                           const std::string& hypothesis);
   };

   TEST_F(AccuracyTest, ClearSpeech) {
       auto audio = load_wav("tests/audio/clear_speech.wav");
       auto result = transcriber.transcribe(audio);

       float wer = calculate_wer(
           "Hello, this is a test of the transcription system.",
           result.text
       );
       EXPECT_LT(wer, 0.1f);  // Less than 10% WER
   }

   TEST_F(AccuracyTest, QuietSpeech) {
       auto audio = load_wav("tests/audio/quiet_speech.wav");
       auto result = transcriber.transcribe(audio);
       EXPECT_FALSE(result.text.empty());
   }

   TEST_F(AccuracyTest, BackgroundNoise) {
       auto audio = load_wav("tests/audio/noisy_speech.wav");
       auto result = transcriber.transcribe(audio);
       EXPECT_GT(result.confidence, 0.6f);
   }

   TEST_F(AccuracyTest, ProperNouns) {
       auto audio = load_wav("tests/audio/proper_nouns.wav");
       auto result = transcriber.transcribe(audio);
       EXPECT_TRUE(result.text.find("Ralph Wiggum") != std::string::npos ||
                   result.text.find("Ralph Wigum") != std::string::npos);
   }

   TEST_F(AccuracyTest, RapidStopStart) {
       // Simulate rapid stop/start
       auto audio1 = load_wav("tests/audio/segment1.wav");
       auto audio2 = load_wav("tests/audio/segment2.wav");

       auto result1 = transcriber.transcribe(audio1);
       auto result2 = transcriber.transcribe(audio2);

       EXPECT_FALSE(result1.text.empty());
       EXPECT_FALSE(result2.text.empty());
       // No duplication
       EXPECT_NE(result1.text, result2.text);
   }
   ```

2. Create test audio files:
   - `tests/audio/clear_speech.wav` - Clear, well-articulated speech
   - `tests/audio/quiet_speech.wav` - Whispered or quiet speech
   - `tests/audio/noisy_speech.wav` - Speech with background noise
   - `tests/audio/proper_nouns.wav` - Speech with names/technical terms
   - `tests/audio/segment1.wav`, `segment2.wav` - Short segments for rapid test

3. Create manual test script `tests/manual_test.sh`:
   ```bash
   #!/bin/bash
   echo "=== Whispr Accuracy Test Suite ==="

   echo -e "\n[Test 1] Clear Speech"
   echo "Say: 'Hello, this is a test of the transcription system.'"
   read -p "Press Enter when ready..."
   # Record and transcribe

   echo -e "\n[Test 2] Quiet Speech"
   echo "Whisper: 'Can you hear me now?'"
   read -p "Press Enter when ready..."

   echo -e "\n[Test 3] Proper Nouns"
   echo "Say: 'My name is Ralph Wiggum and I use Claude from Anthropic.'"
   read -p "Press Enter when ready..."

   echo -e "\n[Test 4] Technical Terms"
   echo "Say: 'I'm writing TypeScript code for the GitHub API.'"
   read -p "Press Enter when ready..."

   echo -e "\n[Test 5] Rapid Stop/Start"
   echo "Record multiple short phrases quickly"
   read -p "Press Enter when ready..."

   echo -e "\n=== Tests Complete ==="
   ```

### Verification
- [ ] All automated tests pass
- [ ] Manual test script works
- [ ] WER < 10% for clear speech
- [ ] WER < 20% for noisy speech
- [ ] Proper nouns recognized 80%+ of time

---

## Phase 7: Performance Benchmarking

### Tasks
1. Create benchmark script to measure:
   - Transcription latency (ms)
   - Word Error Rate (WER)
   - Memory usage
   - CPU/GPU utilization

2. Compare before/after accuracy improvements

3. Document results in `ACCURACY_RESULTS.md`

### Verification
- [ ] Benchmark runs successfully
- [ ] Results documented
- [ ] No significant latency regression (< 50ms increase)

---

## Completion Criteria

All of the following must be true:
- [ ] Whisper parameters optimized per OpenAI recommendations
- [ ] VAD implemented or integrated
- [ ] Audio preprocessing enhanced with AGC
- [ ] Initial prompt system supports custom vocabulary
- [ ] Test framework created with automated and manual tests
- [ ] All automated tests pass
- [ ] Manual tests show improvement
- [ ] WER improved by at least 10% over baseline
- [ ] No latency regression > 50ms
- [ ] Documentation updated

---

## Quick Wins (Implement First)

1. **Lower no_speech_threshold** from 0.5 to 0.3 (5 min)
2. **Add compression_ratio_threshold** of 2.4 (5 min)
3. **Enable temperature fallback** with increment 0.2 (5 min)
4. **Expand vocabulary in initial_prompt** (10 min)

---

## Files to Create/Modify

| File | Action | Description |
|------|--------|-------------|
| `include/config.hpp` | MODIFY | Add PROFILE_OPTIMIZED, new thresholds |
| `src/transcriber.cpp` | MODIFY | Add new whisper parameters |
| `include/vad.hpp` | CREATE | VAD interface |
| `src/vad.cpp` | CREATE | VAD implementation |
| `include/audio_processor.hpp` | MODIFY | Add AGC config |
| `src/audio_processor.cpp` | MODIFY | Implement AGC |
| `~/.whispr/vocabulary.txt` | CREATE | User vocabulary file |
| `tests/test_accuracy.cpp` | CREATE | Automated accuracy tests |
| `tests/manual_test.sh` | CREATE | Manual test script |
| `ACCURACY_RESULTS.md` | CREATE | Benchmark results |

---

## Escape Hatches

### If VAD integration is too complex:
- Use the existing energy-based silence trimming
- Increase silence threshold for better detection
- Consider whisper.cpp's built-in VAD flag if available

### If ONNX runtime is problematic:
- Implement simple energy-based VAD
- Use WebRTC VAD (C library, well-tested)

### If accuracy still insufficient:
- Upgrade to small.en or medium.en model
- Consider faster-whisper for better accuracy
- Add post-processing spell correction

---

## Ralph Loop Execution Command

```bash
/ralph-loop "Implement whisper accuracy improvements following RALPH_WIGGUM_ACCURACY_V2.md.

Execute phases in order:
1. (Optional) Upgrade model if user agrees
2. Optimize whisper parameters (no_speech_thold, compression_ratio, temperature)
3. Implement/integrate VAD
4. Enhance audio preprocessing with AGC
5. Build custom vocabulary system
6. Create comprehensive test framework
7. Run benchmarks and document results

After each phase, verify the checklist items before proceeding.
Run tests after each change to ensure no regression.

Output <promise>ACCURACY_V2_COMPLETE</promise> when:
- All phases implemented
- All automated tests pass
- Manual testing shows improvement
- WER improved by 10%+ over baseline
- Documentation complete

If blocked after 20 iterations:
- Document blocking issue
- List attempted solutions
- Suggest manual intervention needed" --completion-promise "ACCURACY_V2_COMPLETE" --max-iterations 100
```

---

## References

- [whisper.cpp GitHub discussions on accuracy](https://github.com/ggml-org/whisper.cpp/discussions/1035)
- [OpenAI Whisper best practices](https://cookbook.openai.com/examples/whisper_processing_guide)
- [Silero VAD](https://github.com/snakers4/silero-vad)
- [WhisperX for enhanced accuracy](https://github.com/m-bain/whisperX)
- [faster-whisper](https://github.com/SYSTRAN/faster-whisper)
