#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>

namespace rack::com {
    struct RackComContext {
        std::atomic<bool>* cancelled = nullptr;
        std::chrono::steady_clock::time_point deadline;
        std::unordered_map<std::string, std::string> metadata;
    };
}