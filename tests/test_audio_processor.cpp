// Automated tests for AudioProcessor
// Compile: g++ -std=c++17 -I../include -o test_audio test_audio_processor.cpp ../src/audio_processor.cpp -lm

#include "audio_processor.hpp"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace whispr;

// Generate test signals
std::vector<float> generate_sine(int samples, float freq, float amplitude, int sample_rate = 16000) {
    std::vector<float> audio(samples);
    for (int i = 0; i < samples; ++i) {
        audio[i] = amplitude * std::sin(2.0f * M_PI * freq * i / sample_rate);
    }
    return audio;
}

std::vector<float> generate_silence(int samples) {
    return std::vector<float>(samples, 0.0f);
}

std::vector<float> generate_noise(int samples, float amplitude) {
    std::vector<float> audio(samples);
    for (int i = 0; i < samples; ++i) {
        audio[i] = amplitude * (2.0f * (rand() / (float)RAND_MAX) - 1.0f);
    }
    return audio;
}

float calculate_rms(const std::vector<float>& audio) {
    float sum = 0.0f;
    for (float s : audio) sum += s * s;
    return std::sqrt(sum / audio.size());
}

float calculate_peak(const std::vector<float>& audio) {
    float peak = 0.0f;
    for (float s : audio) {
        float abs_s = std::abs(s);
        if (abs_s > peak) peak = abs_s;
    }
    return peak;
}

// Test high-pass filter
void test_highpass() {
    std::cout << "Testing high-pass filter..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Low frequency should be attenuated
    auto low_freq = generate_sine(16000, 30.0f, 0.5f);  // 30Hz - below cutoff
    auto low_before_rms = calculate_rms(low_freq);
    proc.apply_highpass(low_freq);
    auto low_after_rms = calculate_rms(low_freq);

    // High frequency should pass through
    proc.reset();
    auto high_freq = generate_sine(16000, 1000.0f, 0.5f);  // 1kHz - above cutoff
    auto high_before_rms = calculate_rms(high_freq);
    proc.apply_highpass(high_freq);
    auto high_after_rms = calculate_rms(high_freq);

    // Low frequency should be significantly attenuated
    assert(low_after_rms < low_before_rms * 0.5f && "Low frequency should be attenuated");

    // High frequency should pass mostly unchanged
    assert(high_after_rms > high_before_rms * 0.8f && "High frequency should pass through");

    std::cout << "  PASS: High-pass filter working correctly" << std::endl;
}

// Test normalization
void test_normalization() {
    std::cout << "Testing normalization..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Quiet signal should be boosted
    auto quiet = generate_sine(16000, 440.0f, 0.1f);
    proc.apply_normalization(quiet);
    auto peak = calculate_peak(quiet);

    // Should be normalized to target_peak (0.9)
    assert(peak > 0.85f && peak <= 1.0f && "Quiet signal should be normalized to ~0.9");

    std::cout << "  PASS: Normalization boosting quiet signals" << std::endl;

    // Loud signal should not clip
    auto loud = generate_sine(16000, 440.0f, 0.95f);
    proc.apply_normalization(loud);
    peak = calculate_peak(loud);

    assert(peak <= 1.0f && "Loud signal should not clip");

    std::cout << "  PASS: Loud signals not clipping" << std::endl;
}

// Test AGC
void test_agc() {
    std::cout << "Testing AGC..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Quiet signal
    auto quiet = generate_sine(16000, 440.0f, 0.02f);
    auto quiet_rms_before = calculate_rms(quiet);
    proc.apply_agc(quiet);
    auto quiet_rms_after = calculate_rms(quiet);

    // Should be boosted toward target RMS
    assert(quiet_rms_after > quiet_rms_before && "Quiet signal should be boosted");

    std::cout << "  PASS: AGC boosting quiet signals" << std::endl;

    // Loud signal
    auto loud = generate_sine(16000, 440.0f, 0.8f);
    proc.apply_agc(loud);
    auto loud_peak = calculate_peak(loud);

    // Should not clip
    assert(loud_peak <= 1.0f && "AGC should not cause clipping");

    std::cout << "  PASS: AGC not clipping loud signals" << std::endl;
}

// Test silence trimming
void test_silence_trimming() {
    std::cout << "Testing silence trimming..." << std::endl;

    // Create audio with silence at start and end
    auto silence_start = generate_silence(8000);  // 0.5s silence
    auto speech = generate_sine(16000, 440.0f, 0.3f);  // 1s "speech"
    auto silence_end = generate_silence(8000);  // 0.5s silence

    std::vector<float> audio;
    audio.insert(audio.end(), silence_start.begin(), silence_start.end());
    audio.insert(audio.end(), speech.begin(), speech.end());
    audio.insert(audio.end(), silence_end.begin(), silence_end.end());

    auto trimmed = AudioProcessor::trim_silence(audio, 0.01f, 1600, 16000);

    // Trimmed audio should be significantly shorter
    assert(trimmed.size() < audio.size() * 0.8f && "Silence should be trimmed");
    assert(trimmed.size() >= 16000 && "Speech portion should remain");

    std::cout << "  PASS: Silence trimmed correctly" << std::endl;
}

// Test enhanced VAD (extract_speech)
void test_enhanced_vad() {
    std::cout << "Testing enhanced VAD..." << std::endl;

    // Create audio with multiple speech segments
    auto silence1 = generate_silence(4000);  // 0.25s
    auto speech1 = generate_sine(8000, 440.0f, 0.3f);  // 0.5s
    auto silence2 = generate_silence(4000);  // 0.25s
    auto speech2 = generate_sine(8000, 880.0f, 0.4f);  // 0.5s
    auto silence3 = generate_silence(4000);  // 0.25s

    std::vector<float> audio;
    audio.insert(audio.end(), silence1.begin(), silence1.end());
    audio.insert(audio.end(), speech1.begin(), speech1.end());
    audio.insert(audio.end(), silence2.begin(), silence2.end());
    audio.insert(audio.end(), speech2.begin(), speech2.end());
    audio.insert(audio.end(), silence3.begin(), silence3.end());

    auto extracted = AudioProcessor::extract_speech(audio, 0.015f, 100, 50, 16000);

    // Should extract both speech segments
    assert(extracted.size() < audio.size() && "Should remove some silence");
    assert(extracted.size() > 12000 && "Should preserve speech segments");

    std::cout << "  PASS: Enhanced VAD extracting speech" << std::endl;
}

// Test full processing chain
void test_full_chain() {
    std::cout << "Testing full processing chain..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Create realistic test audio: quiet speech with noise
    auto speech = generate_sine(16000, 300.0f, 0.1f);  // Quiet "speech"
    auto noise = generate_noise(16000, 0.01f);  // Quiet noise

    std::vector<float> audio(16000);
    for (size_t i = 0; i < audio.size(); ++i) {
        audio[i] = speech[i] + noise[i];
    }

    auto rms_before = calculate_rms(audio);
    proc.process(audio);
    auto rms_after = calculate_rms(audio);
    auto peak_after = calculate_peak(audio);

    // Audio should be normalized
    assert(rms_after > rms_before && "Audio should be louder after processing");
    assert(peak_after <= 1.0f && "Audio should not clip");

    std::cout << "  PASS: Full processing chain working" << std::endl;
}

int main() {
    std::cout << "\n=== Audio Processor Test Suite ===" << std::endl << std::endl;

    srand(42);  // Reproducible random

    test_highpass();
    test_normalization();
    test_agc();
    test_silence_trimming();
    test_enhanced_vad();
    test_full_chain();

    std::cout << "\n=== All Tests Passed! ===" << std::endl << std::endl;
    return 0;
}
