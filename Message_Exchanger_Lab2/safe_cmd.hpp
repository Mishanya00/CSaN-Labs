#pragma once
#include <mutex>

extern std::mutex io_mutex;

void SafePrint(const std::string& message);
std::string SafeInput();