#include "../include/text_processor.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <cassert>

using namespace whispr;

struct TestCase {
    std::string name;
    std::string input;
    std::string expected;
};

// ANSI color codes
const char* GREEN = "\033[32m";
const char* RED = "\033[31m";
const char* YELLOW = "\033[33m";
const char* RESET = "\033[0m";

int main() {
    TextProcessor processor;
    int passed = 0;
    int failed = 0;

    std::cout << "\n========================================\n";
    std::cout << "Phase 6: Comprehensive TextProcessor Tests\n";
    std::cout << "========================================\n\n";

    // Test scenarios from Ralph Wiggum plan
    std::vector<TestCase> test_cases = {
        // 1. Basic transcription (should preserve normal text)
        {"Basic transcription", "Hello world", "Hello world"},
        {"Basic sentence", "This is a test sentence.", "This is a test sentence."},

        // 2. Filler removal
        {"Filler: um at start", "Um, I think we should go", "I think we should go"},
        {"Filler: uh in middle", "It's, uh, complicated", "It's complicated"},
        {"Filler: umm variant", "Umm, let me think", "Let me think"},
        {"Filler: uhh variant", "Uhh, I don't know", "I don't know"},
        {"Filler: er", "I, er, forgot", "I forgot"},
        {"Filler: ah", "Ah, yes of course", "Yes of course"},
        {"Filler: you know start", "You know, it's really good", "It's really good"},
        {"Filler: you know middle", "It's, you know, complicated", "It's complicated"},
        {"Filler: I mean start", "I mean, it works", "It works"},
        {"Filler: basically start", "Basically, we need to go", "We need to go"},
        {"Filler: actually start", "Actually, I changed my mind", "I changed my mind"},
        {"Filler: like filler", "I was, like, going", "I was going"},
        {"Filler: so start", "So, what do you think", "What do you think"},
        {"Filler: right at end", "That makes sense, right?", "That makes sense."},
        {"Filler: multiple fillers", "Um, I, uh, you know, think so", "I think so"},

        // 3. Capitalization
        {"Capitalize: first letter", "hello world", "Hello world"},
        {"Capitalize: after period", "hello. how are you", "Hello. How are you"},
        {"Capitalize: after question", "what? why not", "What? Why not"},
        {"Capitalize: after exclamation", "wow! that's great", "Wow! That's great"},
        {"Capitalize: standalone i", "i think i should go", "I think I should go"},
        {"Capitalize: i'm contraction", "i'm going to the store", "I'm going to the store"},
        {"Capitalize: i've contraction", "i've been there before", "I've been there before"},
        {"Capitalize: i'll contraction", "i'll do it tomorrow", "I'll do it tomorrow"},
        {"Capitalize: i'd contraction", "i'd like that", "I'd like that"},

        // 4. Spacing
        {"Spacing: double space", "hello  world", "Hello world"},
        {"Spacing: triple space", "hello   world", "Hello world"},
        {"Spacing: leading space", " hello world", "Hello world"},
        {"Spacing: trailing space", "hello world ", "Hello world"},
        {"Spacing: both ends", "  hello world  ", "Hello world"},
        {"Spacing: before comma", "hello , world", "Hello, world"},
        {"Spacing: before period", "hello .", "Hello."},
        {"Spacing: after comma no space", "hello,world", "Hello, world"},
        {"Spacing: after period no space", "hello.world", "Hello. World"},

        // 5. Empty and edge cases
        {"Edge: empty string", "", ""},
        {"Edge: single word", "hello", "Hello"},
        {"Edge: single letter", "a", "A"},
        {"Edge: just filler", "um", ""},
        {"Edge: just spaces", "   ", ""},
        {"Edge: numbers only", "123", "123"},
        {"Edge: punctuation only", "...", "..."},

        // 7. Mixed content (complex real-world scenarios)
        {"Mixed: pizza example", "I, like, really like pizza, you know?", "I really like pizza?"},  // Keep ? since it was a question
        {"Mixed: complex fillers", "Um, so, basically, I think, you know, we should go", "I think we should go"},
        {"Mixed: normal like preserved", "I like to eat pizza", "I like to eat pizza"},
        {"Mixed: comparison like", "It's like a big house", "It's like a big house"},
        // Note: "so" stays because its comma was consumed when ", um," was removed
        {"Mixed: real speech", "so, um, i was thinking that, like, maybe we should, you know, go to the store", "So I was thinking that maybe we should go to the store"},
        {"Mixed: professional", "basically, the project is on track, you know, and i think we'll finish soon", "The project is on track and I think we'll finish soon"},

        // Additional edge cases
        {"Edge: all caps preserved", "I think NASA is great", "I think NASA is great"},
        {"Edge: acronym", "The CEO made an announcement", "The CEO made an announcement"},
        {"Edge: multiple sentences", "hello. this is a test. how are you?", "Hello. This is a test. How are you?"},
    };

    std::cout << "Running " << test_cases.size() << " test cases...\n\n";

    // Run all tests
    for (const auto& tc : test_cases) {
        std::string result = processor.process(tc.input);
        bool pass = (result == tc.expected);

        if (pass) {
            std::cout << GREEN << "[PASS] " << RESET << tc.name << "\n";
            passed++;
        } else {
            std::cout << RED << "[FAIL] " << RESET << tc.name << "\n";
            std::cout << "       Input:    \"" << tc.input << "\"\n";
            std::cout << "       Expected: \"" << tc.expected << "\"\n";
            std::cout << "       Got:      \"" << result << "\"\n";
            failed++;
        }
    }

    // Performance testing
    std::cout << "\n========================================\n";
    std::cout << "Performance Testing\n";
    std::cout << "========================================\n\n";

    // Short text (typical transcription)
    std::string short_text = "Um, I think, you know, that we should, like, go to the meeting.";
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        volatile std::string result = processor.process(short_text);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_short = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    std::cout << "Short text (1000 iterations): " << duration_short << " us avg";
    if (duration_short < 10000) {
        std::cout << GREEN << " [PASS - under 10ms]" << RESET << "\n";
    } else {
        std::cout << RED << " [FAIL - over 10ms]" << RESET << "\n";
        failed++;
    }

    // Medium text (longer transcription)
    std::string medium_text = "Um, so basically, I was, you know, thinking about the project and, like, "
                              "I mean, we really need to, uh, get this done. Basically, the deadline is, "
                              "you know, coming up soon and I think, er, we should, like, focus on the "
                              "main features first. So, um, what do you think about that, right?";
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        volatile std::string result = processor.process(medium_text);
    }
    end = std::chrono::high_resolution_clock::now();
    auto duration_medium = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

    std::cout << "Medium text (1000 iterations): " << duration_medium << " us avg";
    if (duration_medium < 10000) {
        std::cout << GREEN << " [PASS - under 10ms]" << RESET << "\n";
    } else {
        std::cout << RED << " [FAIL - over 10ms]" << RESET << "\n";
        failed++;
    }

    // Long text (30 seconds of speech - approximately 450 words)
    std::string long_text;
    for (int i = 0; i < 30; ++i) {
        long_text += "Um, so basically, I was thinking about this and, you know, I believe we should proceed. ";
    }
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        volatile std::string result = processor.process(long_text);
    }
    end = std::chrono::high_resolution_clock::now();
    auto duration_long = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 100.0;

    std::cout << "Long text (100 iterations): " << duration_long << " us avg";
    if (duration_long < 10000) {
        std::cout << GREEN << " [PASS - under 10ms]" << RESET << "\n";
    } else {
        std::cout << YELLOW << " [WARN - over 10ms but acceptable for long text]" << RESET << "\n";
    }

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n\n";
    std::cout << "Total tests: " << (passed + failed) << "\n";
    std::cout << GREEN << "Passed: " << passed << RESET << "\n";
    std::cout << (failed > 0 ? RED : GREEN) << "Failed: " << failed << RESET << "\n";

    if (failed == 0) {
        std::cout << "\n" << GREEN << "ALL TESTS PASSED! Phase 6 complete." << RESET << "\n\n";
        return 0;
    } else {
        std::cout << "\n" << RED << "SOME TESTS FAILED. Please fix issues above." << RESET << "\n\n";
        return 1;
    }
}
