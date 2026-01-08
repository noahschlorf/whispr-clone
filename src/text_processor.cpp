#include "text_processor.hpp"
#include <cctype>
#include <regex>

namespace whispr {

TextProcessor::TextProcessor(const TextProcessorConfig& config)
    : config_(config) {}

std::string TextProcessor::process(const std::string& text) const {
    if (text.empty()) return text;

    std::string result = text;

    // Order matters: remove fillers first, then fix spacing, then capitalize, then punctuation
    if (config_.remove_fillers) {
        result = remove_filler_words(result);
    }

    if (config_.fix_spacing) {
        result = fix_spacing(result);
    }

    if (config_.auto_capitalize) {
        result = fix_capitalization(result);
    }

    if (config_.trim_whitespace) {
        result = trim(result);
    }

    if (config_.ensure_punctuation) {
        result = ensure_punctuation(result);
    }

    return result;
}

std::string TextProcessor::remove_filler_words(const std::string& text) const {
    std::string result = text;

    // Helper regex patterns (static for performance)
    // Extended um/uh/er/ah patterns - catches "um", "umm", "ummm", "uh", "uhh", "uhhh", etc.
    static const std::regex simple_fillers_with_comma(
        R"(,?\s*\b[Uu]+[HhMm]+\b,?\s*|,?\s*\b[Uu]+[Hh]+\b,?\s*|,?\s*\b[Ee]+[Rr]+\b,?\s*|,?\s*\b[Aa]+[Hh]+\b,?\s*|,?\s*\b[Hh][Mm]+\b,?\s*)",
        std::regex::ECMAScript
    );
    static const std::regex you_know(
        R"(,?\s*\b[Yy]ou know\b,?\s*)",
        std::regex::ECMAScript
    );
    static const std::regex i_mean(
        R"((^|[.!?]\s*)[Ii] mean,?\s*)",
        std::regex::ECMAScript
    );
    // Catch ", like," and also " like " when preceded by certain words (filler pattern)
    static const std::regex like_filler_commas(
        R"(,\s*like,\s*)",
        std::regex::ECMAScript | std::regex::icase
    );
    // Catch "like" as filler in specific safe patterns:
    // - "was like" when not followed by "a/an/the" (comparison)
    // - "is like" when not followed by "a/an/the"
    static const std::regex like_filler_after_words(
        R"(\b(was|is)\s+like\s+(?!(a|an|the|that|this|what|how|who)\b))",
        std::regex::ECMAScript | std::regex::icase
    );
    // Catch "like" at start of sentence (filler)
    static const std::regex like_start_filler(
        R"(^[Ll]ike\s+(?=[a-z]))",
        std::regex::ECMAScript
    );
    // Catch "so like" at start
    static const std::regex so_like_start(
        R"(^[Ss]o\s+like\s+)",
        std::regex::ECMAScript
    );
    // Catch "and like" / "but like" / "or like" mid-sentence
    static const std::regex conj_like_filler(
        R"(\b(and|but|or)\s+like\s+(?=[a-z]))",
        std::regex::ECMAScript | std::regex::icase
    );
    // Catch "should/could/would/might like <verb>" (filler before verb)
    static const std::regex modal_like_verb(
        R"(\b(should|could|would|might|must|can|will)\s+like\s+(?=[a-z]+\b))",
        std::regex::ECMAScript | std::regex::icase
    );
    // Catch "so" at start when it's a filler (followed by common patterns)
    static const std::regex so_start_filler(
        R"(^[Ss]o\s+(?=I\s|we\s|you\s|they\s|he\s|she\s|it\s|the\s|a\s|an\s|basically|actually|um|uh))",
        std::regex::ECMAScript
    );
    static const std::regex right_end(
        R"(,\s*right\s*[.?]?\s*$)",
        std::regex::ECMAScript | std::regex::icase
    );
    static const std::regex double_comma(R"(,\s*,)");
    static const std::regex double_space(R"(\s{2,})");
    static const std::regex leading_ws(R"(^\s+)");
    static const std::regex orphan_comma_start(R"(^\s*,\s*)");
    // Catch repeated/stuttered words (e.g., "I I" or "the the")
    static const std::regex stuttered_words(
        R"(\b(\w+)\s+\1\b)",
        std::regex::ECMAScript | std::regex::icase
    );

    // Run filler removal in a loop since removing one filler may expose another
    bool filler_changed = true;
    int filler_iterations = 0;
    while (filler_changed && filler_iterations < 5) {
        filler_changed = false;
        filler_iterations++;
        std::string prev = result;

        // Remove simple fillers (um, uh, uhh, er, ah, hmm)
        result = std::regex_replace(result, simple_fillers_with_comma, " ");
        // Remove "you know" phrase
        result = std::regex_replace(result, you_know, " ");
        // Remove "I mean" at start or after punctuation
        result = std::regex_replace(result, i_mean, "$1");
        // Remove ", like," as filler
        result = std::regex_replace(result, like_filler_commas, " ");
        // Remove "like" after was/is when not a comparison
        result = std::regex_replace(result, like_filler_after_words, "$1 ");
        // Remove "like" at sentence start
        result = std::regex_replace(result, like_start_filler, "");
        // Remove "so like" at start
        result = std::regex_replace(result, so_like_start, "");
        // Remove "and/but/or like" mid-sentence
        result = std::regex_replace(result, conj_like_filler, "$1 ");
        // Remove "should/could/would like" before verb
        result = std::regex_replace(result, modal_like_verb, "$1 ");
        // Remove "so" at start when it's a filler
        result = std::regex_replace(result, so_start_filler, "");
        // Clean up spacing
        result = std::regex_replace(result, double_space, " ");
        result = std::regex_replace(result, leading_ws, "");

        if (result != prev) {
            filler_changed = true;
        }
    }

    // Remove "right" at end (tag question filler)
    result = std::regex_replace(result, right_end, ".");

    // Remove stuttered/repeated words
    result = std::regex_replace(result, stuttered_words, "$1");

    // Final cleanup of spacing and commas
    result = std::regex_replace(result, double_comma, ",");
    result = std::regex_replace(result, double_space, " ");
    result = std::regex_replace(result, leading_ws, "");
    result = std::regex_replace(result, orphan_comma_start, "");

    // Step 7: Remove start-of-sentence fillers (so, basically, actually, like)
    // Run in a loop since removing one might expose another
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 5) {
        changed = false;
        iterations++;
        std::string prev = result;

        // "so," at start (only with comma - "so" alone might be meaningful)
        if (result.size() >= 3 && (result[0] == 'S' || result[0] == 's') &&
            result[1] == 'o' && result[2] == ',') {
            size_t pos = 3;
            while (pos < result.size() && std::isspace(static_cast<unsigned char>(result[pos]))) pos++;
            result = result.substr(pos);
            changed = true;
        }
        // "basically" at start
        else if (result.size() >= 9 &&
            (result.substr(0, 9) == "Basically" || result.substr(0, 9) == "basically")) {
            size_t pos = 9;
            if (pos < result.size() && result[pos] == ',') pos++;
            while (pos < result.size() && std::isspace(static_cast<unsigned char>(result[pos]))) pos++;
            result = result.substr(pos);
            changed = true;
        }
        // "actually" at start
        else if (result.size() >= 8 &&
            (result.substr(0, 8) == "Actually" || result.substr(0, 8) == "actually")) {
            size_t pos = 8;
            if (pos < result.size() && result[pos] == ',') pos++;
            while (pos < result.size() && std::isspace(static_cast<unsigned char>(result[pos]))) pos++;
            result = result.substr(pos);
            changed = true;
        }
        // "like," at start
        else if (result.size() >= 5 &&
            (result.substr(0, 5) == "Like," || result.substr(0, 5) == "like,")) {
            size_t pos = 5;
            while (pos < result.size() && std::isspace(static_cast<unsigned char>(result[pos]))) pos++;
            result = result.substr(pos);
            changed = true;
        }
    }

    // Final cleanup
    result = std::regex_replace(result, double_space, " ");
    result = std::regex_replace(result, leading_ws, "");

    return result;
}

std::string TextProcessor::fix_capitalization(const std::string& text) const {
    if (text.empty()) return text;

    std::string result = text;
    bool capitalize_next = true;

    for (size_t i = 0; i < result.size(); ++i) {
        char c = result[i];

        if (capitalize_next && std::isalpha(static_cast<unsigned char>(c))) {
            result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize_next = false;
        } else if (c == '.' || c == '!' || c == '?') {
            capitalize_next = true;
        }
    }

    // Fix standalone 'i' -> 'I'
    // Match ' i ' or start 'i ' or ' i' end or "i'" (i'm, i've, i'll, i'd)
    static const std::regex standalone_i(R"((?:^|\s)i(?:\s|'|$))");

    std::string fixed;
    fixed.reserve(result.size() + 10);

    std::sregex_iterator it(result.begin(), result.end(), standalone_i);
    std::sregex_iterator end;

    size_t last_pos = 0;
    while (it != end) {
        std::smatch match = *it;
        size_t match_start = static_cast<size_t>(match.position());
        size_t match_len = static_cast<size_t>(match.length());

        // Append text before match
        fixed.append(result, last_pos, match_start - last_pos);

        // Append match with 'i' -> 'I'
        std::string matched = match.str();
        for (char& mc : matched) {
            if (mc == 'i') mc = 'I';
        }
        fixed.append(matched);

        last_pos = match_start + match_len;
        ++it;
    }

    // Append remaining text
    fixed.append(result, last_pos, result.size() - last_pos);

    return fixed;
}

std::string TextProcessor::fix_spacing(const std::string& text) const {
    if (text.empty()) return text;

    std::string result;
    result.reserve(text.size());

    bool last_was_space = true; // Start true to trim leading spaces
    bool last_was_punctuation = false;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            // Only add space if last char wasn't space and we're not at the end
            if (!last_was_space) {
                result += ' ';
                last_was_space = true;
            }
            last_was_punctuation = false;
        } else if (c == '.' || c == ',' || c == '!' || c == '?' || c == ':' || c == ';') {
            // Remove space before punctuation
            if (!result.empty() && result.back() == ' ') {
                result.pop_back();
            }
            result += c;
            last_was_space = false;
            last_was_punctuation = true;
        } else {
            // Regular character
            // Add space after punctuation if needed
            if (last_was_punctuation && !last_was_space && c != '\'' && c != '"') {
                result += ' ';
            }
            result += c;
            last_was_space = false;
            last_was_punctuation = false;
        }
    }

    return result;
}

std::string TextProcessor::trim(const std::string& text) const {
    if (text.empty()) return text;

    size_t start = 0;
    size_t end = text.size();

    // Find first non-whitespace
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    // Find last non-whitespace
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    if (start >= end) return "";

    return text.substr(start, end - start);
}

std::string TextProcessor::ensure_punctuation(const std::string& text) const {
    if (text.empty()) return text;

    std::string result = text;

    // Trim trailing whitespace first
    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }

    if (result.empty()) return result;

    // Check if already ends with punctuation
    char last = result.back();
    if (last == '.' || last == '!' || last == '?' || last == ':' || last == ';') {
        return result;
    }

    // Add period if text doesn't end with punctuation
    result += '.';
    return result;
}

} // namespace whispr
