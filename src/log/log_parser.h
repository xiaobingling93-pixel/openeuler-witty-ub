#pragma once

#include "failure_def.h"
#include "log_template.h"

namespace failure::log {
    class LogParser {
    public:
        void AddFailureMode(const FailureMode& mode);
        std::optional<std::pair<LogTemplate, std::unordered_map<std::string, std::string>>> MatchMultiLineTemplate(const std::string& line) const;
        std::optional<std::pair<LogTemplate, std::unordered_map<std::string, std::string>>> MatchSingleLineTemplate(const std::string& line) const;

    private:
        std::vector<LogTemplate> multiLineTemplates_;
        std::vector<LogTemplate> singleLineTemplates_;
    };
}