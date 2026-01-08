#pragma once

#include <string>
#include <vector>

namespace whispr {

// User vocabulary configuration
struct VocabularyConfig {
    std::vector<std::string> proper_nouns;     // Names, places, products
    std::vector<std::string> technical_terms;  // Technical/domain terms
    std::vector<std::string> common_phrases;   // Frequently used phrases

    bool empty() const {
        return proper_nouns.empty() && technical_terms.empty() && common_phrases.empty();
    }
};

class VocabularyLoader {
public:
    // Load vocabulary from ~/.whispr/vocabulary.txt
    static VocabularyConfig load_user_vocabulary();

    // Load vocabulary from specified path
    static VocabularyConfig load_from_file(const std::string& path);

    // Build initial prompt from vocabulary config
    // Optimized for whisper's 224 token limit
    static std::string build_initial_prompt(const VocabularyConfig& vocab,
                                            const std::string& base_prompt = "");

    // Get default vocabulary file path
    static std::string get_default_vocabulary_path();

    // Create default vocabulary file with examples
    static bool create_default_vocabulary_file();

private:
    // Truncate to approximate token count (rough estimate: 4 chars = 1 token)
    static std::string truncate_to_tokens(const std::string& text, int max_tokens);
};

} // namespace whispr
