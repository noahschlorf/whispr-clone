# Ralph Wiggum Plan: Text Formatting & Cleanup

## Overview

Add intelligent text post-processing to whispr-clone that:
1. Removes filler words (um, uh, like, you know, etc.)
2. Auto-formats text (capitalization, spacing, punctuation)
3. Fixes rapid tap/re-record formatting issues
4. Keeps changes subtle and natural-sounding

## Current State

- **Location**: `/Users/noahschlorf/claude/whispr-clone/src/transcriber.cpp`
- **Current Flow**:
  1. Whisper transcribes audio → raw text
  2. Basic whitespace trim
  3. Text sent to clipboard and pasted
- **Problems**:
  - Filler words appear in output ("um", "like", "uh", etc.)
  - Rapid re-recording can cause formatting glitches
  - No sentence capitalization or cleanup
  - Leading/trailing spaces sometimes appear

## Target State

- Clean, professional transcription output
- Filler words automatically removed
- Proper sentence capitalization
- Clean spacing (no double spaces, proper punctuation spacing)
- Seamless rapid re-recording without formatting issues

---

## Phase 1: Create Text Processor Module

### Tasks
1. Create `include/text_processor.hpp` header
2. Create `src/text_processor.cpp` implementation
3. Add to CMakeLists.txt

### File: `include/text_processor.hpp`
```cpp
#pragma once
#include <string>
#include <vector>
#include <regex>

namespace whispr {

struct TextProcessorConfig {
    bool remove_fillers = true;
    bool auto_capitalize = true;
    bool fix_spacing = true;
    bool trim_whitespace = true;
};

class TextProcessor {
public:
    TextProcessor() = default;
    explicit TextProcessor(const TextProcessorConfig& config);

    // Main processing function
    std::string process(const std::string& text) const;

    // Individual operations (for testing)
    std::string remove_filler_words(const std::string& text) const;
    std::string fix_capitalization(const std::string& text) const;
    std::string fix_spacing(const std::string& text) const;
    std::string trim(const std::string& text) const;

    void set_config(const TextProcessorConfig& config) { config_ = config; }

private:
    TextProcessorConfig config_;

    // Filler word patterns
    static const std::vector<std::string>& get_filler_patterns();
};

} // namespace whispr
```

### Filler Words to Remove
- "um", "umm", "ummm"
- "uh", "uhh", "uhhh"
- "er", "err"
- "ah", "ahh"
- "like" (when used as filler, not comparison)
- "you know"
- "I mean"
- "basically"
- "actually" (when filler)
- "sort of", "kind of" (optional, more aggressive)
- "right" (when filler at end)
- "so" (at start of sentence, optional)

### Filler Removal Strategy
- Use word boundary regex: `\b(um|uh|er|ah)+\b`
- Handle "like" carefully - only remove when:
  - At start of sentence
  - After comma with no following comparison
  - Repeated ("like like")
- Preserve meaning: "I like pizza" stays, "I was like, going" → "I was going"

### Verification
- [ ] Header file created with proper interface
- [ ] Implementation file created
- [ ] CMakeLists.txt updated
- [ ] Compiles without errors

---

## Phase 2: Implement Filler Word Removal

### Tasks
1. Define filler word patterns (regex-based)
2. Implement `remove_filler_words()` function
3. Handle edge cases (preserve meaning)
4. Add unit tests

### Implementation Notes
```cpp
std::string TextProcessor::remove_filler_words(const std::string& text) const {
    std::string result = text;

    // Simple fillers (always remove)
    // Pattern: word boundary + filler + word boundary
    static const std::regex simple_fillers(
        R"(\b(u+[hm]+|e+r+|a+h+)\b)",
        std::regex::icase
    );
    result = std::regex_replace(result, simple_fillers, "");

    // "you know" - remove
    static const std::regex you_know(R"(\byou know\b,?\s*)", std::regex::icase);
    result = std::regex_replace(result, you_know, "");

    // "I mean" at start - remove
    static const std::regex i_mean_start(R"(^I mean,?\s*)", std::regex::icase);
    result = std::regex_replace(result, i_mean_start, "");

    // "like" as filler (tricky - be conservative)
    // Only remove ", like," or "like," at sentence start
    static const std::regex like_filler(R"(,\s*like,\s*)", std::regex::icase);
    result = std::regex_replace(result, like_filler, ", ");

    // "basically" at start
    static const std::regex basically_start(R"(^basically,?\s*)", std::regex::icase);
    result = std::regex_replace(result, basically_start, "");

    return result;
}
```

### Test Cases
- "Um, I think we should go" → "I think we should go"
- "I was like, going to the store" → "I was going to the store"
- "You know, it's really good" → "It's really good"
- "I like pizza" → "I like pizza" (preserved!)
- "It's, uh, complicated" → "It's complicated"
- "Basically, I think..." → "I think..."

### Verification
- [ ] Simple fillers (um, uh, er, ah) removed
- [ ] "you know" removed
- [ ] "I mean" at start removed
- [ ] "like" preserved when it's a verb
- [ ] "like" removed when it's a filler
- [ ] No double spaces created

---

## Phase 3: Implement Auto-Formatting

### Tasks
1. Implement `fix_capitalization()` - capitalize first letter of sentences
2. Implement `fix_spacing()` - clean up spaces around punctuation
3. Chain processors in correct order

### Capitalization Rules
- Capitalize after: . ! ?
- Capitalize at start of text
- Handle "I" - always capitalize
- Don't over-capitalize (preserve acronyms, proper nouns)

### Spacing Rules
- Single space after . ! ? ,
- No space before . ! ? ,
- No double spaces
- No leading/trailing spaces
- Handle ellipsis (...) correctly

### Implementation Notes
```cpp
std::string TextProcessor::fix_capitalization(const std::string& text) const {
    if (text.empty()) return text;

    std::string result = text;
    bool capitalize_next = true;

    for (size_t i = 0; i < result.size(); ++i) {
        char c = result[i];

        if (capitalize_next && std::isalpha(c)) {
            result[i] = std::toupper(c);
            capitalize_next = false;
        } else if (c == '.' || c == '!' || c == '?') {
            capitalize_next = true;
        }

        // Always capitalize 'i' when standalone
        if (c == 'i' && i > 0 && i < result.size() - 1) {
            if (!std::isalpha(result[i-1]) && !std::isalpha(result[i+1])) {
                result[i] = 'I';
            }
        }
    }

    return result;
}

std::string TextProcessor::fix_spacing(const std::string& text) const {
    std::string result;
    result.reserve(text.size());

    bool last_was_space = true; // Trim leading

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (std::isspace(c)) {
            if (!last_was_space && i < text.size() - 1) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            // Remove space before punctuation
            if ((c == '.' || c == ',' || c == '!' || c == '?') &&
                !result.empty() && result.back() == ' ') {
                result.pop_back();
            }
            result += c;
            last_was_space = false;

            // Add space after punctuation if not already there
            if ((c == '.' || c == ',' || c == '!' || c == '?') &&
                i + 1 < text.size() && !std::isspace(text[i + 1])) {
                result += ' ';
                last_was_space = true;
            }
        }
    }

    // Trim trailing
    while (!result.empty() && std::isspace(result.back())) {
        result.pop_back();
    }

    return result;
}
```

### Test Cases
- "hello world" → "Hello world"
- "hello. world" → "Hello. World"
- "i think i should go" → "I think I should go"
- "hello  world" → "Hello world" (double space fixed)
- " hello " → "Hello" (trimmed)
- "hello ,world" → "Hello, world"
- "what?why" → "What? Why"

### Verification
- [ ] First letter capitalized
- [ ] Letters after . ! ? capitalized
- [ ] Standalone "i" → "I"
- [ ] Double spaces removed
- [ ] Punctuation spacing correct
- [ ] Leading/trailing whitespace removed

---

## Phase 4: Integrate into Transcriber

### Tasks
1. Add TextProcessor to Transcriber class
2. Call processor after whisper transcription
3. Update Config to include processing options

### Changes to `transcriber.hpp`
```cpp
#include "text_processor.hpp"

class Transcriber {
    // ... existing members ...

    void set_text_processing(bool enabled) { process_text_ = enabled; }
    void set_text_processor_config(const TextProcessorConfig& config);

private:
    TextProcessor text_processor_;
    bool process_text_ = true;
};
```

### Changes to `transcriber.cpp` (in transcribe())
```cpp
// After getting raw text from whisper...

// Post-process text
if (process_text_ && !text.empty()) {
    text = text_processor_.process(text);
}

result.text = text;
```

### Verification
- [ ] TextProcessor integrated
- [ ] Processing can be enabled/disabled
- [ ] Config can be customized
- [ ] Transcription still works end-to-end

---

## Phase 5: Fix Rapid Re-Recording Issues

### Tasks
1. Identify source of formatting glitches
2. Add proper state cleanup between recordings
3. Ensure clipboard is cleared/set atomically

### Root Causes to Address
- Previous transcription text interfering
- Clipboard not being cleared properly
- Race conditions in paste timing

### Fixes
1. Clear audio buffer completely on new recording
2. Add small delay before paste (already exists, verify sufficient)
3. Ensure state is fully reset before new recording

### Changes to `app.cpp`
```cpp
void App::start_recording() {
    if (state_.load() != AppState::Idle) {
        // If still transcribing, wait or cancel
        return;
    }

    // Clear any previous audio data
    audio_->clear_buffer();  // Add this method if needed

    std::cout << "Recording..." << std::endl;
    state_.store(AppState::Recording);
    update_tray_state(AppState::Recording);

    audio_->start_recording();
}
```

### Verification
- [ ] Rapid tap-record-tap-record works cleanly
- [ ] No ghost text from previous transcription
- [ ] No formatting artifacts between recordings

---

## Phase 6: Testing & Polish

### Tasks
1. Manual testing of all scenarios
2. Edge case testing
3. Performance verification (processing should be <10ms)
4. Code cleanup and documentation

### Test Scenarios
1. **Basic transcription**: "Hello world" → "Hello world"
2. **Filler removal**: "Um, hello, uh, world" → "Hello world"
3. **Capitalization**: "hello. how are you" → "Hello. How are you"
4. **Rapid recording**: Record → Stop → Record immediately → Clean output
5. **Empty recording**: Short tap → No crash, no garbage output
6. **Long recording**: 30 seconds of speech → Proper formatting throughout
7. **Mixed content**: "I, like, really like pizza, you know?" → "I really like pizza"

### Performance Requirements
- Text processing: <10ms for typical transcription
- No noticeable delay in paste operation
- No memory leaks in processing

### Verification
- [ ] All test scenarios pass
- [ ] Performance meets requirements
- [ ] Code is clean and documented
- [ ] No regressions in existing functionality

---

## Completion Criteria

All of the following must be true:

- [ ] TextProcessor module created and integrated
- [ ] Filler words (um, uh, like*, you know, etc.) removed
- [ ] Text auto-capitalized (sentences, standalone I)
- [ ] Spacing fixed (no double spaces, proper punctuation)
- [ ] Rapid re-recording works without formatting issues
- [ ] Build passes without warnings
- [ ] Manual testing of all scenarios passes
- [ ] Processing time <10ms

---

## Ralph Loop Execution Command

```bash
/ralph-wiggum "Implement text formatting for whispr-clone following RALPH_WIGGUM_TEXT_FORMATTING.md.

Current: Raw whisper output with filler words and formatting issues
Target: Clean, professional text with fillers removed and proper formatting

Execute phases 1-6 in order:
1. Create TextProcessor module (header + implementation)
2. Implement filler word removal (um, uh, like, you know, etc.)
3. Implement auto-formatting (capitalization, spacing)
4. Integrate into Transcriber
5. Fix rapid re-recording issues
6. Test all scenarios

After each phase, verify the checklist items before proceeding.

Output <promise>TEXT_FORMATTING_COMPLETE</promise> when:
- TextProcessor module working
- Filler words being removed
- Auto-formatting working
- Rapid re-recording clean
- All tests pass

If blocked after 20 iterations:
- Document blocking issue
- List what was attempted
- Suggest manual steps needed"
```

---

## Files to Create/Modify

| File | Action | Description |
|------|--------|-------------|
| `include/text_processor.hpp` | CREATE | TextProcessor class header |
| `src/text_processor.cpp` | CREATE | TextProcessor implementation |
| `CMakeLists.txt` | MODIFY | Add text_processor.cpp |
| `include/transcriber.hpp` | MODIFY | Add TextProcessor member |
| `src/transcriber.cpp` | MODIFY | Call TextProcessor |
| `src/app.cpp` | MODIFY | Fix rapid re-recording |

---

## Escape Hatches

### If regex is too slow:
- Use simple string find/replace instead
- Pre-compile regex patterns (static)
- Process in chunks for very long text

### If capitalization is wrong:
- Make it configurable (off by default)
- Only capitalize after explicit sentence endings
- Don't touch text that's already mixed case

### If "like" removal is too aggressive:
- Remove only the most obvious cases
- Require comma before/after
- Make it a config option

---

## Notes

- Keep changes subtle - don't over-process
- Preserve original meaning always
- Performance is critical - this runs on every transcription
- Test with real speech patterns, not just clean sentences
- Consider adding config to disable any feature user doesn't want
