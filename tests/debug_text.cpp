#include "text_processor.hpp"
#include <iostream>

using namespace whispr;

int main() {
    TextProcessor proc;
    
    std::string inputs[] = {
        "Um, hello there",
        "Hello um there",
        "hello there",
        "I really like pizza"
    };
    
    for (const auto& input : inputs) {
        std::string result = proc.process(input);
        std::cout << "Input:  \"" << input << "\"" << std::endl;
        std::cout << "Output: \"" << result << "\"" << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}
