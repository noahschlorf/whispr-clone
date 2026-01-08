#pragma once

#include <vector>
#include <cmath>

namespace whispr {

// Audio preprocessing configuration
struct AudioProcessorConfig {
    // High-pass filter to remove low-frequency rumble
    bool enable_highpass = true;
    float highpass_freq = 80.0f;  // Hz - removes HVAC, rumble, etc.

    // Noise gate to suppress quiet background noise
    bool enable_noise_gate = true;
    float noise_gate_threshold = 0.02f;  // ~-34dB
    float noise_gate_attack = 0.001f;    // seconds
    float noise_gate_release = 0.05f;    // seconds

    // Normalization for consistent levels
    bool enable_normalization = true;
    float target_peak = 0.9f;  // Peak normalization target
};

// Audio preprocessing for improved transcription accuracy
class AudioProcessor {
public:
    using Config = AudioProcessorConfig;

    explicit AudioProcessor(float sample_rate = 16000.0f);
    AudioProcessor(float sample_rate, const Config& config);

    // Process audio buffer in-place
    void process(std::vector<float>& audio);

    // Individual processing stages (for testing)
    void apply_highpass(std::vector<float>& audio);
    void apply_noise_gate(std::vector<float>& audio);
    void apply_normalization(std::vector<float>& audio);

    // Reset filter state (call between recordings)
    void reset();

    void set_config(const Config& config) { config_ = config; reset(); }
    const Config& get_config() const { return config_; }

private:
    Config config_;
    float sample_rate_;

    // Biquad high-pass filter coefficients
    float b0_, b1_, b2_, a1_, a2_;

    // Filter state (for continuity between buffers)
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;

    // Noise gate state
    float gate_env_ = 0.0f;

    void design_highpass_filter();
};

} // namespace whispr
