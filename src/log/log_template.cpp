/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * witty-ub is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define MODULE_NAME "LOG"

#include "log_template.h"

#include "logger.h"

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

    std::optional<FailureEvent> LogTemplate::CreateEvent(
        std::unordered_map<std::string, std::string>&& attributes,
        std::string&& line
    ) const
    {
        auto it = attributes.find("datetime");
        if (it == attributes.end()) {
            return std::nullopt;
        }
        auto timestamp = failure::DatetimeStrToTimestamp(it->second);
        if (!timestamp) {
            return std::nullopt;
        }

        FailureEvent event;
        event.timestamp = *timestamp;
        event.component = mode_.component;
        event.text = std::move(line);
        event.attributes = std::move(attributes);
        return event;
    }

    std::string LogTemplate::Escape(const std::string& str)
    {
        return re2::RE2::QuoteMeta(str);
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
            } else {
                size_t rangeStart = fieldExpr.find('(');
                if (rangeStart == std::string::npos) {
                    fields_.push_back(fieldExpr);
                    patternStr += "(.*)";
                } else {
                    size_t rangeEnd = fieldExpr.find(')', rangeStart);
                    if (rangeEnd == std::string::npos) {
                        LOG_WARN << "invalid field range: " << fieldExpr;
                        break;
                    }
                    std::string fieldName = fieldExpr.substr(0, rangeStart);
                    fields_.push_back(fieldName);
                    std::string rangeExpr = fieldExpr.substr(rangeStart + 1, rangeEnd - rangeStart - 1);
                    std::vector<std::string> range;
                    failure::Split(range, rangeExpr, '/', /*keepEmpty=*/true);
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
        pattern_ = std::make_unique<re2::RE2>(patternStr, re2::RE2::Options());
        if (!pattern_->ok()) {
            LOG_ERROR << "failed to compile log template regex: " << pattern_->error()
                      << ", pattern: " << patternStr;
        }
    }

    std::optional<std::unordered_map<std::string, std::string>> LogTemplate::CaptureFields(const std::string& line) const
    {
        if (!pattern_ || !pattern_->ok()) {
            return std::nullopt;
        }

        std::vector<re2::StringPiece> groups(fields_.size() + 1);
        re2::StringPiece input(line);
        if (pattern_->Match(input, 0, input.size(), re2::RE2::UNANCHORED, groups.data(), groups.size())) {
            std::unordered_map<std::string, std::string> res;
            if (fields_.size() != groups.size() - 1) {
                LOG_ERROR << "the size of fields to be matched (" << fields_.size()
                          << ") does not match the number of captured ones (" << groups.size() - 1 << ")";
                return std::nullopt;
            }
            res.reserve(fields_.size());
            for (size_t i = 1; i < groups.size(); ++i) {
                res.emplace(fields_[i - 1], std::string(groups[i]));
            }
            return res;
        }
        return std::nullopt;
    }
}
