#include "safe_cmd.hpp"
#include <iostream>
#include <string>

std::mutex io_mutex;

void SafePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(io_mutex);
    std::cout << message;        
}

std::string SafeInput() {
    std::lock_guard<std::mutex> lock(io_mutex);
    std::string input;
    std::getline(std::cin, input);
    return input;
}