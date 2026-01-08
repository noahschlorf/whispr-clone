#pragma once

#include <string>

namespace whispr {

struct TextProcessorConfig {
    bool remove_fillers = true;
    bool auto_capitalize = true;
    bool fix_spacing = true;
    bool trim_whitespace = true;
    bool ensure_punctuation = true;  // Add period if sentence doesn't end with punctuation
};

class TextProcessor {
public:
    TextProcessor() = default;
    explicit TextProcessor(const TextProcessorConfig& config);

    // Main processing function - applies all enabled transformations
    std::string process(const std::string& text) const;

    // Individual operations (public for testing)
    std::string remove_filler_words(const std::string& text) const;
    std::string fix_capitalization(const std::string& text) const;
    std::string fix_spacing(const std::string& text) const;
    std::string trim(const std::string& text) const;
    std::string ensure_punctuation(const std::string& text) const;

    void set_config(const TextProcessorConfig& config) { config_ = config; }
    const TextProcessorConfig& get_config() const { return config_; }

private:
    TextProcessorConfig config_;
};

} // namespace whispr
