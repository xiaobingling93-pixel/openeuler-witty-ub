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

#include "log_reader.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

#include "logger.h"

namespace failure::log {
    static constexpr int VEC_SCALER = 2;

    std::string TrimCopy(const std::string& str)
    {
        std::size_t begin = 0;
        while (begin < str.size() && std::isspace(static_cast<unsigned char>(str[begin])) != 0) {
            ++begin;
        }

        std::size_t end = str.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(str[end - 1])) != 0) {
            --end;
        }
        return str.substr(begin, end - begin);
    }

    bool HasKeywordChar(const std::string& str)
    {
        return std::any_of(str.begin(), str.end(), [](unsigned char ch) {
            return std::isalnum(ch) != 0 || ch == '_';
        });
    }

    std::string ShellEscapeSingleQuotes(const std::string& str)
    {
        std::string escaped;
        escaped.reserve(str.size());
        for (char ch : str) {
            if (ch == '\'') {
                escaped += "'\\''";
            } else {
                escaped += ch;
            }
        }
        return escaped;
    }

    std::vector<std::string> ExtractManifestKeywords(const std::string& manifest)
    {
        std::vector<std::string> keywords;
        auto addKeyword = [&keywords](const std::string& candidate) {
            std::string trimmed = TrimCopy(candidate);
            if (trimmed.empty() || !HasKeywordChar(trimmed)) {
                return;
            }
            if (std::find(keywords.begin(), keywords.end(), trimmed) == keywords.end()) {
                keywords.push_back(std::move(trimmed));
            }
        };

        std::size_t pos = 0;
        while (pos < manifest.size()) {
            std::size_t start = manifest.find('<', pos);
            std::string literal = manifest.substr(pos, start == std::string::npos ? std::string::npos : start - pos);

            std::size_t partStart = 0;
            while (partStart <= literal.size()) {
                std::size_t partEnd = literal.find('|', partStart);
                size_t partEndPos = partEnd == std::string::npos ? std::string::npos : partEnd - partStart;
                addKeyword(literal.substr(partStart, partEndPos));
                if (partEnd == std::string::npos) {
                    break;
                }
                partStart = partEnd + 1;
            }

            if (start == std::string::npos) {
                break;
            }
            std::size_t end = manifest.find('>', start + 1);
            if (end == std::string::npos) {
                break;
            }
            pos = end + 1;
        }
        return keywords;
    }

    std::string BuildKeywordFilter(const std::vector<std::string>& keywords)
    {
        if (keywords.empty()) {
            return "";
        }

        std::string filter = " | grep -F";
        for (const std::string& keyword : keywords) {
            filter += " -e '";
            filter += ShellEscapeSingleQuotes(keyword);
            filter += "'";
        }
        return filter;
    }

    LogReader::LogReader(DataSourceOption option, const PathCell& pathCell, int64_t startTime, int64_t endTime)
        : option_(option),
        pathCell_(pathCell),
        startTime_(startTime),
        endTime_(endTime),
        handle_(nullptr),
        parser_(std::make_unique<LogParser>())
    {
    }

    LogReader::~LogReader()
    {
        DestroyHandle();
    }

    void LogReader::CreateHandle()
    {
        if (!opener_ || !closer_) {
            ConfigureHandle(option_);
        }
        if (!handle_ && opener_) {
            handle_ = opener_(pathCell_.path);
        }
    }

    void LogReader::DestroyHandle()
    {
        if (handle_ && closer_) {
            closer_(handle_);
            handle_ = nullptr;
        }
    }

    void LogReader::AddFailureMode(const FailureMode& mode)
    {
        parser_->AddFailureMode(mode);
        for (const std::string& keyword : ExtractManifestKeywords(mode.manifest)) {
            if (std::find(keywords_.begin(), keywords_.end(), keyword) == keywords_.end()) {
                keywords_.push_back(keyword);
            }
        }
    }

    std::optional<FailureEvent> LogReader::ReadOnce()
    {
        while (true) {
            auto line = ReadNextLine();
            if (!line) {
                return std::nullopt;
            }

            if (auto entry = parser_->MatchSingleLineTemplate(*line)) {
                const LogTemplate& tmpl = *entry->first;
                std::unordered_map<std::string, std::string>& attributes = entry->second;
                if (auto event = tmpl.CreateEvent(std::move(attributes), std::move(*line))) {
                    event->pathCell = pathCell_;
                    return event;
                }
            } else if (auto entry = parser_->MatchMultiLineTemplate(*line)) {
                const LogTemplate* tmpl = entry->first;
                std::unordered_map<std::string, std::string>& attributes = entry->second;
                auto it = attributes.find("identifier");
                std::string identifier = it->second;
                std::string lines;
                lines.reserve(line->size() + readBufSize_);
                lines.append(*line);
                while (auto nextLine = ReadNextLine()) {
                    auto nextAttributes = tmpl->Match(*nextLine);
                    if (nextAttributes &&
                        nextAttributes->find("identifier") != nextAttributes->end() &&
                        nextAttributes->at("identifier") == identifier) {
                        if (lines.capacity() < lines.size() + nextLine->size()) {
                            lines.reserve((lines.size() + nextLine->size()) * VEC_SCALER);
                        }
                        lines.append(*nextLine);
                        continue;
                    }
                    cachedLine_ = std::move(nextLine);
                    break;
                }
                attributes.erase(it);
                if (auto event = tmpl->CreateEvent(std::move(attributes), std::move(lines))) {
                    event->pathCell = pathCell_;
                    return event;
                }
            }
        }

        return std::nullopt;
    }

    std::optional<std::string> LogReader::ReadNextLine()
    {
        if (cachedLine_) {
            auto line = std::move(*cachedLine_);
            cachedLine_.reset();
            return line;
        }

        lineBuffer_.clear();
        while (fgets(readBuffer_.data(), readBuffer_.size(), handle_) != nullptr) {
            const std::size_t chunkLen = std::strlen(readBuffer_.data());
            lineBuffer_.append(readBuffer_.data(), chunkLen);

            if (chunkLen > 0 && readBuffer_[chunkLen - 1] == '\n') {
                return std::move(lineBuffer_);
            }
        }
        if (lineBuffer_.empty()) {
            return std::nullopt;
        }

        return std::move(lineBuffer_);
    }

    void LogReader::ConfigureHandle(DataSourceOption option)
    {
        if (option == DataSourceOption::KERNEL) {
            opener_ = [&](const std::string& p) {
                std::string script = R"(
        BEGIN {
            mon["Jan"]=1; mon["Feb"]=2; mon["Mar"]=3; mon["Apr"]=4;
            mon["May"]=5; mon["Jun"]=6; mon["Jul"]=7; mon["Aug"]=8;
            mon["Sep"]=9; mon["Oct"]=10; mon["Nov"]=11; mon["Dec"]=12;
        }
        {
            if (index($1, "[") != 1) next;
            year = $5; sub(/\]/, "", year);
            split($4, t, ":");
            ts = mktime(year " " mon[$2] " " $3 " " t[1] " " t[2] " " t[3]);
            if (ts < s) next;
            if (ts > e) exit;
            print $0;
        }
    )";
                int64_t startSec = startTime_ / 1000000;
                int64_t endSec = endTime_ / 1000000;
                std::string cmd = "gawk -v s=" + std::to_string(startSec) + " -v e=" + std::to_string(endSec) + " -e '" + script + "' ";
                if (std::filesystem::is_regular_file(p)) {
                    cmd += p;
                } else {
                    cmd = p + " -T | " + cmd;
                }
                cmd += BuildKeywordFilter(keywords_);
                return popen(cmd.c_str(), "r");
                };
            closer_ = [](FILE* f) {
                pclose(f);
                };
        } else {
            opener_ = [&](const std::string& p) {
                auto startTimeStr = failure::TimestampToDatetimeStr(startTime_, "iso8601");
                auto endTimeStr = failure::TimestampToDatetimeStr(endTime_, "iso8601");
                std::string cmd = "awk -F'|' '$1 >= \"" + *startTimeStr + ".000000+08:00\" && $1 <= \"" + *endTimeStr + ".999999+08:00\"' " + p;
                cmd += BuildKeywordFilter(keywords_);
                return popen(cmd.c_str(), "r");
                };
            closer_ = [](FILE* f) {
                pclose(f);
                };
        }
    }
}
