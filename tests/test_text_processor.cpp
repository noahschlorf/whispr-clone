// Automated tests for TextProcessor
// Compile: g++ -std=c++17 -I../include -o test_text test_text_processor.cpp ../src/text_processor.cpp

#include "text_processor.hpp"
#include <iostream>
#include <cassert>

using namespace whispr;

void test_basic_filler_removal() {
    std::cout << "Testing basic filler removal..." << std::endl;

    TextProcessor proc;

    // Test "um" removal (note: processor adds period at end)
    assert(proc.process("Um, hello there") == "Hello there.");
    assert(proc.process("Hello um there") == "Hello there.");

    // Test "uh" removal
    std::string result = proc.process("Uh, what was I saying");
    assert(result.find("uh") == std::string::npos && result.find("Uh") == std::string::npos);

    // Test "you know" removal
    result = proc.process("So, you know, it's fine");
    assert(result.find("you know") == std::string::npos);

    std::cout << "  PASS" << std::endl;
}

void test_like_removal() {
    std::cout << "Testing 'like' filler removal..." << std::endl;

    TextProcessor proc;

    // Should remove "like" as filler in certain contexts
    std::string result = proc.process("It was like really good");
    // Note: "was like" is a filler pattern
    assert(result.find("like really") == std::string::npos || result.find("was really") != std::string::npos);

    // Should NOT remove "like" as verb
    result = proc.process("I really like pizza");
    assert(result.find("like") != std::string::npos && "Verb 'like' should be preserved");

    std::cout << "  PASS" << std::endl;
}

void test_stuttering_removal() {
    std::cout << "Testing stuttering removal..." << std::endl;

    TextProcessor proc;

    std::string result = proc.process("I I think so");
    assert(result.find("I I") == std::string::npos && "Should remove stuttered 'I I'");

    result = proc.process("The the quick brown fox");
    assert(result.find("The the") == std::string::npos && result.find("the the") == std::string::npos);

    std::cout << "  PASS" << std::endl;
}

void test_capitalization() {
    std::cout << "Testing capitalization..." << std::endl;

    TextProcessor proc;

    // First word should be capitalized
    std::string result = proc.process("hello there");
    assert(result[0] == 'H' && "First character should be uppercase");

    // Proper nouns (I) should be capitalized
    result = proc.process("and i think so");
    assert(result.find(" I ") != std::string::npos && "Standalone 'I' should be capitalized");

    std::cout << "  PASS" << std::endl;
}

void test_punctuation() {
    std::cout << "Testing punctuation..." << std::endl;

    TextProcessor proc;

    // Should add period if missing
    std::string result = proc.process("Hello there");
    assert(!result.empty() && (result.back() == '.' || result.back() == '!' || result.back() == '?')
           && "Should end with punctuation");

    // Should not add extra punctuation
    result = proc.process("Hello there.");
    int periods = 0;
    for (char c : result) if (c == '.') periods++;
    assert(periods == 1 && "Should not add duplicate punctuation");

    std::cout << "  PASS" << std::endl;
}

void test_whitespace_cleanup() {
    std::cout << "Testing whitespace cleanup..." << std::endl;

    TextProcessor proc;

    // Multiple spaces should become single space
    std::string result = proc.process("Hello    there");
    assert(result.find("  ") == std::string::npos && "No double spaces");

    // Space before punctuation should be removed
    result = proc.process("Hello ,there");
    assert(result.find(" ,") == std::string::npos && "No space before comma");

    std::cout << "  PASS" << std::endl;
}

void test_complex_sentences() {
    std::cout << "Testing complex sentences..." << std::endl;

    TextProcessor proc;

    // Complex sentence with multiple issues
    std::string input = "um so like, i was thinking that, you know, we could um try this";
    std::string result = proc.process(input);

    // Should remove fillers and capitalize
    assert(result[0] == 'S' || result[0] == 'I' && "Should start with capital");
    assert(result.find("um") == std::string::npos && "No 'um'");
    assert(result.find("you know") == std::string::npos && "No 'you know'");

    std::cout << "  Result: \"" << result << "\"" << std::endl;
    std::cout << "  PASS" << std::endl;
}

void test_edge_cases() {
    std::cout << "Testing edge cases..." << std::endl;

    TextProcessor proc;

    // Empty string
    assert(proc.process("") == "");

    // Just fillers
    std::string result = proc.process("um uh");
    // Result might be empty or minimal after removing all fillers

    // Just whitespace
    assert(proc.process("   ") == "");

    std::cout << "  PASS" << std::endl;
}

int main() {
    std::cout << "\n=== Text Processor Test Suite ===" << std::endl << std::endl;

    test_basic_filler_removal();
    test_like_removal();
    test_stuttering_removal();
    test_capitalization();
    test_punctuation();
    test_whitespace_cleanup();
    test_complex_sentences();
    test_edge_cases();

    std::cout << "\n=== All Tests Passed! ===" << std::endl << std::endl;
    return 0;
}
