#include "vocabulary.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

namespace whispr {

std::string VocabularyLoader::get_default_vocabulary_path() {
    const char* home = std::getenv("HOME");
    if (!home) return "";
    return std::string(home) + "/.whispr/vocabulary.txt";
}

VocabularyConfig VocabularyLoader::load_user_vocabulary() {
    return load_from_file(get_default_vocabulary_path());
}

VocabularyConfig VocabularyLoader::load_from_file(const std::string& path) {
    VocabularyConfig vocab;

    if (path.empty()) return vocab;

    std::ifstream file(path);
    if (!file.is_open()) {
        // File doesn't exist - that's OK, user hasn't created one yet
        return vocab;
    }

    enum class Section { None, ProperNouns, TechnicalTerms, CommonPhrases };
    Section current_section = Section::None;

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            // Check for section headers in comments
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find("proper noun") != std::string::npos ||
                lower.find("names") != std::string::npos) {
                current_section = Section::ProperNouns;
            } else if (lower.find("technical") != std::string::npos ||
                       lower.find("term") != std::string::npos) {
                current_section = Section::TechnicalTerms;
            } else if (lower.find("phrase") != std::string::npos ||
                       lower.find("common") != std::string::npos) {
                current_section = Section::CommonPhrases;
            }
            continue;
        }

        // Add to appropriate section
        switch (current_section) {
            case Section::ProperNouns:
                vocab.proper_nouns.push_back(line);
                break;
            case Section::TechnicalTerms:
                vocab.technical_terms.push_back(line);
                break;
            case Section::CommonPhrases:
                vocab.common_phrases.push_back(line);
                break;
            case Section::None:
                // Default to proper nouns if no section specified
                vocab.proper_nouns.push_back(line);
                break;
        }
    }

    std::cout << "Loaded vocabulary: " << vocab.proper_nouns.size() << " proper nouns, "
              << vocab.technical_terms.size() << " technical terms, "
              << vocab.common_phrases.size() << " phrases" << std::endl;

    return vocab;
}

std::string VocabularyLoader::truncate_to_tokens(const std::string& text, int max_tokens) {
    // Rough estimate: 4 characters per token on average
    int max_chars = max_tokens * 4;
    if (static_cast<int>(text.length()) <= max_chars) {
        return text;
    }

    // Find last complete word before limit
    std::string truncated = text.substr(0, max_chars);
    size_t last_space = truncated.find_last_of(" ,.");
    if (last_space != std::string::npos && last_space > 0) {
        truncated = truncated.substr(0, last_space);
    }

    return truncated;
}

std::string VocabularyLoader::build_initial_prompt(const VocabularyConfig& vocab,
                                                    const std::string& base_prompt) {
    std::ostringstream prompt;

    // Start with base prompt if provided
    if (!base_prompt.empty()) {
        prompt << base_prompt;
        if (base_prompt.back() != ' ' && base_prompt.back() != '.') {
            prompt << " ";
        }
    }

    // Add proper nouns
    if (!vocab.proper_nouns.empty()) {
        prompt << "Names and proper nouns: ";
        for (size_t i = 0; i < vocab.proper_nouns.size(); ++i) {
            if (i > 0) prompt << ", ";
            prompt << vocab.proper_nouns[i];
        }
        prompt << ". ";
    }

    // Add technical terms
    if (!vocab.technical_terms.empty()) {
        prompt << "Technical terms: ";
        for (size_t i = 0; i < vocab.technical_terms.size(); ++i) {
            if (i > 0) prompt << ", ";
            prompt << vocab.technical_terms[i];
        }
        prompt << ". ";
    }

    // Add common phrases (these can be particularly helpful)
    if (!vocab.common_phrases.empty()) {
        prompt << "Common phrases: ";
        for (size_t i = 0; i < vocab.common_phrases.size(); ++i) {
            if (i > 0) prompt << "; ";
            prompt << "\"" << vocab.common_phrases[i] << "\"";
        }
        prompt << ". ";
    }

    // Truncate to stay within token limit (leave some room for safety)
    return truncate_to_tokens(prompt.str(), 200);
}

bool VocabularyLoader::create_default_vocabulary_file() {
    std::string path = get_default_vocabulary_path();
    if (path.empty()) return false;

    // Create directory if needed
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (!std::filesystem::exists(dir)) {
        try {
            std::filesystem::create_directories(dir);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create vocabulary directory: " << e.what() << std::endl;
            return false;
        }
    }

    // Don't overwrite existing file
    if (std::filesystem::exists(path)) {
        return true;  // File already exists
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to create vocabulary file: " << path << std::endl;
        return false;
    }

    file << R"(# Whispr Vocabulary File
# Add words and phrases here to improve transcription accuracy.
# Whisper will use these as hints for better recognition.
#
# Sections are detected by keywords in comments:
# - "proper nouns" or "names" for names, places, products
# - "technical terms" for domain-specific vocabulary
# - "common phrases" for frequently used expressions

# Proper nouns - names of people, places, products
Ralph Wiggum
Claude
Anthropic
macOS

# Technical terms - domain-specific vocabulary
API
GitHub
TypeScript
JavaScript
Python
whisper.cpp

# Common phrases - expressions you frequently use
Let me think about this
That makes sense
Could you please
I'd like to
)";

    std::cout << "Created vocabulary file: " << path << std::endl;
    return true;
}

} // namespace whispr
