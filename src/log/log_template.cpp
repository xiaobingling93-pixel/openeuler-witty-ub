#define MODULE_NAME "LOG"

#include "log_template.h"

#include "logger.h"
#include "utils.h"

namespace failure::log {
    LogTemplate::LogTemplate(const FailureMode& mode)
        : mode_(mode)
    {
        CreateRegexCaptor(mode.manifest);
    }

    std::optional<std::unordered_map<std::string, std::string>> LogTemplate::Match(const std::string& line) const
    {
        if (auto attributes = CaptureFields(line)) {
            return attributes;
        }
        return std::nullopt;
    }

    std::optional<FailureEvent> LogTemplate::CreateEvent(std::unordered_map<std::string, std::string>& attributes, const std::string& line) const
    {
        auto it = attributes.find("datetime");
        if (it == attributes.end()) {
            return std::nullopt;
        }
        auto timestamp = utils::DatetimeStrToTimestamp(it->second);
        if (!timestamp) {
            return std::nullopt;
        }

        FailureEvent event;
        event.timestamp = *timestamp;
        event.component = mode_.component;
        event.text = line;
        event.attributes = attributes;
        return event;
    }

    std::string LogTemplate::Escape(const std::string& str)
    {
        static const std::regex escapeRegexChar(R"([.^$|()\\[\]{}*+?])");
        return std::regex_replace(str, escapeRegexChar, R"(\$&)");
    }

    void LogTemplate::CreateRegexCaptor(const std::string& manifest)
    {
        std::string patternStr;
        size_t pos = 0;
        while (true) {
            size_t start = manifest.find("<", pos);
            if (start == std::string::npos) {
                patternStr += Escape(manifest.substr(pos));
                break;
            }
            patternStr += Escape(manifest.substr(pos, start - pos));
            size_t end = manifest.find(">", start + 1);
            if (end == std::string::npos) {
                LOG_WARN << "unclosed field name pack < >: " << manifest;
                break;
            }
            std::string fieldExpr = manifest.substr(start + 1, end - start - 1);
            if (fieldExpr.empty()) {
                patternStr += ".*";
            }
            else {
                size_t rangeStart = fieldExpr.find('(');
                if (rangeStart == std::string::npos) {
                    fields_.push_back(fieldExpr);
                    patternStr += "(.*)";
                }
                else {
                    size_t rangeEnd = fieldExpr.find(')', rangeStart);
                    if (rangeEnd == std::string::npos) {
                        LOG_WARN << "invalid field range: " << fieldExpr;
                        break;
                    }
                    std::string fieldName = fieldExpr.substr(0, rangeStart);
                    fields_.push_back(fieldName);
                    std::string rangeExpr = fieldExpr.substr(rangeStart + 1, rangeEnd - rangeStart - 1);
                    std::vector<std::string> range;
                    utils::Split(range, rangeExpr, '/', /*keepEmpty=*/true);
                    std::string group = "(";
                    for (int i = 0; i < range.size(); ++i) {
                        if (!range[i].empty()) {
                            group += Escape(range[i]);
                        }
                        if (i != range.size() - 1) {
                            group += '|';
                        }
                    }
                    group += ')';
                    patternStr += group;
                }
            }

            pos = end + 1;
        }
        pattern_ = std::regex(patternStr, std::regex::ECMAScript | std::regex::optimize);
    }

    std::optional<std::unordered_map<std::string, std::string>> LogTemplate::CaptureFields(const std::string& line) const
    {
        std::smatch match;
        if (std::regex_search(line, match, pattern_)) {
            std::unordered_map<std::string, std::string> res;
            if (fields_.size() != match.size() - 1) {
                LOG_ERROR << "the size of fields to be matched (" << fields_.size() << ") does not match the number of captured ones (" << match.size() - 1 << ")";
                return std::nullopt;
            }
            for (size_t i = 1; i < match.size(); ++i) {
                res[fields_[i - 1]] = match.str(i);
            }
            return res;
        }
        return std::nullopt;
    }
}