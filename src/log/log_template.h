#pragma once

#include <optional>
#include <regex>

#include "failure_def.h"

namespace failure::log {
    class LogTemplate {
    public:
        LogTemplate(const FailureMode& mode);

        std::optional<std::unordered_map<std::string, std::string>> Match(const std::string& line) const;
        std::optional<FailureEvent> CreateEvent(std::unordered_map<std::string, std::string>& attributes, const std::string& line) const;

    private:
        static std::string Escape(const std::string& str);

        void CreateRegexCaptor(const std::string& manifest);
        std::optional<std::unordered_map<std::string, std::string>> CaptureFields(const std::string& line) const;
    
    private:
        FailureMode mode_;
        std::regex pattern_;
        std::vector<std::string> fields_;
    };
}