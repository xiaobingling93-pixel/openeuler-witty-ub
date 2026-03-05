#define MODULE_NAME "LOG"

#include "log_parser.h"

#include "logger.h"

namespace failure::log {
    void LogParser::AddFailureMode(const FailureMode& mode)
    {
        if (mode.isMultiline) {
            multiLineTemplates_.emplace_back(mode);
        }
        else {
            singleLineTemplates_.emplace_back(mode);
        }
    }

    std::optional<std::pair<LogTemplate, std::unordered_map<std::string, std::string>>> LogParser::MatchMultiLineTemplate(const std::string& line) const
    {
        for (const LogTemplate& tmpl : multiLineTemplates_) {
            if (auto attributes = tmpl.Match(line)) {
                return std::make_pair(std::cref(tmpl), *attributes);
            }
        }
        return std::nullopt;
    }

    std::optional<std::pair<LogTemplate, std::unordered_map<std::string, std::string>>> LogParser::MatchSingleLineTemplate(const std::string& line) const
    {
        for (const LogTemplate& tmpl : singleLineTemplates_) {
            if (auto attributes = tmpl.Match(line)) {
                return std::make_pair(std::cref(tmpl), *attributes);
            }
        }
        return std::nullopt;
    }
}