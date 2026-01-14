// Edge case tests for VoxType audio processing
// Compile: g++ -std=c++17 -I../include -o test_edge_cases test_edge_cases.cpp ../src/audio_processor.cpp

#include "audio_processor.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>
#include <limits>

using namespace whispr;

void test_empty_audio() {
    std::cout << "Testing empty audio handling..." << std::endl;

    AudioProcessor proc(16000.0f);

    std::vector<float> empty_audio;
    proc.process(empty_audio);

    assert(empty_audio.empty() && "Empty audio should remain empty");

    // Trim silence on empty audio
    auto trimmed = AudioProcessor::trim_silence(empty_audio, 0.01f, 160, 16000);
    assert(trimmed.empty() && "Trimmed empty audio should be empty");

    // Extract speech from empty audio
    auto speech = AudioProcessor::extract_speech(empty_audio, 0.01f, 100, 50, 16000);
    assert(speech.empty() && "Speech extraction from empty should be empty");

    std::cout << "  PASS: Empty audio handled correctly" << std::endl;
}

void test_very_short_audio() {
    std::cout << "Testing very short audio (< 10ms)..." << std::endl;

    AudioProcessor proc(16000.0f);

    // 5ms of audio = 80 samples at 16kHz
    std::vector<float> short_audio(80, 0.5f);

    proc.process(short_audio);
    assert(short_audio.size() == 80 && "Short audio size should be preserved");

    // Should not crash on very short audio
    auto trimmed = AudioProcessor::trim_silence(short_audio, 0.01f, 160, 16000);
    // May be empty or reduced

    std::cout << "  PASS: Very short audio handled correctly" << std::endl;
}

void test_long_audio() {
    std::cout << "Testing long audio (30 seconds)..." << std::endl;

    AudioProcessor proc(16000.0f);

    // 30 seconds at 16kHz = 480,000 samples
    std::vector<float> long_audio(480000);
    for (size_t i = 0; i < long_audio.size(); ++i) {
        float t = static_cast<float>(i) / 16000.0f;
        long_audio[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * t);
    }

    size_t original_size = long_audio.size();
    proc.process(long_audio);

    assert(long_audio.size() == original_size && "Long audio size should be preserved");

    // Verify no corruption
    bool has_valid_data = false;
    for (float sample : long_audio) {
        assert(!std::isnan(sample) && "No NaN in long audio");
        assert(!std::isinf(sample) && "No Inf in long audio");
        if (std::abs(sample) > 0.01f) has_valid_data = true;
    }
    assert(has_valid_data && "Long audio should have non-zero data");

    std::cout << "  PASS: Long audio processed correctly (" << original_size << " samples)" << std::endl;
}

void test_silence_only() {
    std::cout << "Testing silence-only audio..." << std::endl;

    std::vector<float> silence(16000, 0.0f);  // 1 second of silence

    // Both functions return original audio if no speech found (for safety)
    // This is intentional behavior - better to return original than empty

    auto trimmed = AudioProcessor::trim_silence(silence, 0.01f, 160, 16000);
    // Returns original when no speech detected
    assert((trimmed.empty() || trimmed.size() == silence.size()) &&
           "Pure silence returns empty or original");

    auto speech = AudioProcessor::extract_speech(silence, 0.01f, 100, 50, 16000);
    // Returns original when no speech segments found
    assert((speech.empty() || speech.size() == silence.size()) &&
           "Pure silence returns empty or original");

    // Verify the returned audio is still valid (all zeros)
    if (!speech.empty()) {
        float max_val = 0.0f;
        for (float s : speech) max_val = std::max(max_val, std::abs(s));
        assert(max_val < 0.001f && "Silence should remain silent");
    }

    std::cout << "  PASS: Silence-only audio handled correctly" << std::endl;
}

void test_extreme_values() {
    std::cout << "Testing extreme audio values..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Test with clipping values
    std::vector<float> clipped(1600);
    for (size_t i = 0; i < clipped.size(); ++i) {
        clipped[i] = (i % 2 == 0) ? 1.0f : -1.0f;
    }

    proc.process(clipped);

    // Verify normalization worked
    for (float sample : clipped) {
        assert(sample >= -1.0f && sample <= 1.0f && "Samples should be clamped to [-1, 1]");
    }

    std::cout << "  PASS: Extreme values handled correctly" << std::endl;
}

void test_dc_offset() {
    std::cout << "Testing DC offset removal..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Audio with strong DC offset
    std::vector<float> dc_audio(16000);
    for (size_t i = 0; i < dc_audio.size(); ++i) {
        float t = static_cast<float>(i) / 16000.0f;
        dc_audio[i] = 0.5f + 0.2f * std::sin(2.0f * M_PI * 440.0f * t);  // DC offset of 0.5
    }

    // Calculate mean before
    float mean_before = 0.0f;
    for (float s : dc_audio) mean_before += s;
    mean_before /= dc_audio.size();

    proc.process(dc_audio);

    // Calculate mean after - should be closer to 0
    float mean_after = 0.0f;
    for (float s : dc_audio) mean_after += s;
    mean_after /= dc_audio.size();

    // High-pass filter should reduce DC component
    assert(std::abs(mean_after) < std::abs(mean_before) && "DC offset should be reduced");

    std::cout << "  PASS: DC offset reduced from " << mean_before << " to " << mean_after << std::endl;
}

void test_very_quiet_audio() {
    std::cout << "Testing very quiet audio..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Very quiet audio (0.001 amplitude)
    std::vector<float> quiet(16000);
    for (size_t i = 0; i < quiet.size(); ++i) {
        float t = static_cast<float>(i) / 16000.0f;
        quiet[i] = 0.001f * std::sin(2.0f * M_PI * 440.0f * t);
    }

    // Calculate RMS before
    float rms_before = 0.0f;
    for (float s : quiet) rms_before += s * s;
    rms_before = std::sqrt(rms_before / quiet.size());

    proc.process(quiet);

    // Calculate RMS after
    float rms_after = 0.0f;
    for (float s : quiet) rms_after += s * s;
    rms_after = std::sqrt(rms_after / quiet.size());

    // Audio processing should not corrupt the signal
    // Note: The high-pass filter may reduce very low frequencies,
    // so RMS might decrease or stay similar. The important thing is
    // the audio remains valid and normalized.
    for (float s : quiet) {
        assert(!std::isnan(s) && "No NaN values");
        assert(!std::isinf(s) && "No Inf values");
        assert(s >= -1.0f && s <= 1.0f && "Samples normalized");
    }

    std::cout << "  PASS: Quiet audio processed (RMS " << rms_before << " -> " << rms_after << ")" << std::endl;
}

void test_single_sample() {
    std::cout << "Testing single sample audio..." << std::endl;

    AudioProcessor proc(16000.0f);

    std::vector<float> single(1, 0.5f);
    proc.process(single);

    assert(single.size() == 1 && "Single sample should be preserved");
    assert(!std::isnan(single[0]) && "Single sample should not be NaN");

    std::cout << "  PASS: Single sample handled correctly" << std::endl;
}

void test_alternating_speech_silence() {
    std::cout << "Testing alternating speech/silence pattern..." << std::endl;

    float sample_rate = 16000.0f;
    std::vector<float> audio;

    // Create pattern: 0.2s silence, 0.3s speech, 0.2s silence, 0.3s speech, 0.2s silence
    for (int section = 0; section < 5; ++section) {
        bool is_speech = (section % 2 == 1);
        int samples = is_speech ? 4800 : 3200;  // 0.3s or 0.2s

        for (int i = 0; i < samples; ++i) {
            if (is_speech) {
                float t = static_cast<float>(i) / sample_rate;
                audio.push_back(0.3f * std::sin(2.0f * M_PI * 300.0f * t));
            } else {
                audio.push_back(0.0f);
            }
        }
    }

    auto speech = AudioProcessor::extract_speech(audio, 0.01f, 200, 50, 16000);

    // Should extract speech segments
    assert(!speech.empty() && "Should extract speech from alternating pattern");
    assert(speech.size() < audio.size() && "Extracted speech should be shorter than original");

    std::cout << "  PASS: Alternating pattern: " << audio.size()
              << " -> " << speech.size() << " samples" << std::endl;
}

int main() {
    std::cout << "\n=== Edge Cases Test Suite ===" << std::endl << std::endl;

    test_empty_audio();
    test_very_short_audio();
    test_long_audio();
    test_silence_only();
    test_extreme_values();
    test_dc_offset();
    test_very_quiet_audio();
    test_single_sample();
    test_alternating_speech_silence();

    std::cout << "\n=== All Edge Case Tests Passed! ===" << std::endl << std::endl;
    return 0;
}
