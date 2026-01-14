// Integration tests for the VoxType audio-to-text pipeline
// Compile: g++ -std=c++17 -I../include -o test_integration test_integration.cpp ../src/audio_processor.cpp ../src/text_processor.cpp

#include "audio_processor.hpp"
#include "text_processor.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

using namespace whispr;

// Generate synthetic audio with a specific pattern
std::vector<float> generate_test_audio(float duration_sec, float sample_rate, float frequency = 440.0f) {
    int num_samples = static_cast<int>(duration_sec * sample_rate);
    std::vector<float> audio(num_samples);

    for (int i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        audio[i] = 0.3f * std::sin(2.0f * M_PI * frequency * t);
    }

    return audio;
}

// Generate audio with speech-like characteristics (varying amplitude)
std::vector<float> generate_speech_like_audio(float duration_sec, float sample_rate) {
    int num_samples = static_cast<int>(duration_sec * sample_rate);
    std::vector<float> audio(num_samples);

    for (int i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        // Simulate speech with varying amplitude envelope
        float envelope = 0.5f * (1.0f + std::sin(2.0f * M_PI * 3.0f * t)); // 3Hz modulation
        audio[i] = 0.2f * envelope * std::sin(2.0f * M_PI * 200.0f * t);
    }

    return audio;
}

void test_audio_processor_pipeline() {
    std::cout << "Testing audio processor pipeline..." << std::endl;

    AudioProcessor proc(16000.0f);

    // Generate 2 seconds of audio
    auto audio = generate_speech_like_audio(2.0f, 16000.0f);
    size_t original_size = audio.size();

    // Process through full pipeline
    proc.process(audio);

    // Verify output is still valid
    assert(!audio.empty() && "Audio should not be empty after processing");
    assert(audio.size() == original_size && "Audio size should not change");

    // Verify no NaN or Inf values
    for (float sample : audio) {
        assert(!std::isnan(sample) && "No NaN values allowed");
        assert(!std::isinf(sample) && "No Inf values allowed");
        assert(sample >= -1.0f && sample <= 1.0f && "Samples should be normalized");
    }

    std::cout << "  PASS: Audio processor pipeline working correctly" << std::endl;
}

void test_silence_trimming_pipeline() {
    std::cout << "Testing silence trimming pipeline..." << std::endl;

    float sample_rate = 16000.0f;

    // Create audio with silence at start and end
    std::vector<float> audio;

    // 0.5s silence
    for (int i = 0; i < 8000; ++i) audio.push_back(0.0f);

    // 1s of "speech"
    for (int i = 0; i < 16000; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        audio.push_back(0.3f * std::sin(2.0f * M_PI * 300.0f * t));
    }

    // 0.5s silence
    for (int i = 0; i < 8000; ++i) audio.push_back(0.0f);

    size_t original_size = audio.size();

    // Trim silence
    auto trimmed = AudioProcessor::trim_silence(audio, 0.01f, 160, 16000);

    assert(trimmed.size() < original_size && "Trimmed audio should be shorter");
    assert(!trimmed.empty() && "Trimmed audio should not be empty");

    std::cout << "  PASS: Silence trimming reduced " << original_size
              << " samples to " << trimmed.size() << std::endl;
}

void test_text_processor_integration() {
    std::cout << "Testing text processor with various inputs..." << std::endl;

    TextProcessor proc;

    // Test multiple sentences
    std::vector<std::pair<std::string, bool>> test_cases = {
        {"hello world", true},
        {"um so basically", true},
        {"I I think so", true},
        {"", true},  // Empty should not crash
        {"a", true},  // Single char should not crash
        {std::string(1000, 'a'), true},  // Long string should not crash
    };

    for (const auto& [input, should_succeed] : test_cases) {
        try {
            std::string result = proc.process(input);
            assert(should_succeed && "Processing should succeed");
        } catch (const std::exception& e) {
            assert(!should_succeed && "Unexpected exception");
        }
    }

    std::cout << "  PASS: Text processor handles all input types" << std::endl;
}

void test_full_pipeline_simulation() {
    std::cout << "Testing full pipeline simulation..." << std::endl;

    // Simulate the full pipeline without actual transcription

    // 1. Generate audio
    auto audio = generate_speech_like_audio(3.0f, 16000.0f);
    std::cout << "  Generated " << audio.size() << " samples" << std::endl;

    // 2. Process audio
    AudioProcessor audio_proc(16000.0f);
    audio_proc.process(audio);
    std::cout << "  Audio processing complete" << std::endl;

    // 3. Trim silence
    auto trimmed = AudioProcessor::extract_speech(audio, 0.02f, 200, 50, 16000);
    std::cout << "  Speech extraction complete: " << trimmed.size() << " samples" << std::endl;

    // 4. Verify minimum length
    int min_samples = 16000 / 10;  // 100ms
    if (static_cast<int>(trimmed.size()) < min_samples) {
        trimmed.resize(min_samples, 0.0f);
    }
    std::cout << "  Padded to minimum: " << trimmed.size() << " samples" << std::endl;

    // 5. Text processing (simulate transcription result)
    TextProcessor text_proc;
    std::string simulated_transcription = "um hello, you know, this is a test";
    std::string cleaned = text_proc.process(simulated_transcription);
    std::cout << "  Text processed: \"" << cleaned << "\"" << std::endl;

    assert(!cleaned.empty() && "Cleaned text should not be empty");
    assert(cleaned.find("um") == std::string::npos && "Fillers should be removed");

    std::cout << "  PASS: Full pipeline simulation successful" << std::endl;
}

int main() {
    std::cout << "\n=== Integration Test Suite ===" << std::endl << std::endl;

    test_audio_processor_pipeline();
    test_silence_trimming_pipeline();
    test_text_processor_integration();
    test_full_pipeline_simulation();

    std::cout << "\n=== All Integration Tests Passed! ===" << std::endl << std::endl;
    return 0;
}
