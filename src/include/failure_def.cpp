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

#include "failure_def.h"

#include <array>
#include <chrono>
#include <ctime>
#include <cstring>

namespace failure {
    constexpr std::size_t BUFFER_SIZE = 64;

    bool FailureEventQuery::Match(const FailureMetadata& metadata, bool podMode) const
    {
        if (!eventTypes.empty() && eventTypes.find(metadata.eventType) == eventTypes.end()) {
            return false;
        }
        if (podMode && !podIds.empty() && metadata.podId && podIds.find(*metadata.podId) == podIds.end()) {
            return false;
        }
        if (!localEids.empty() && localEids.find(metadata.localEid) == localEids.end()) {
            return false;
        }
        if (!jettyIds.empty() && jettyIds.find(metadata.localJettyId) == jettyIds.end()) {
            return false;
        }
        return true;
    }

    std::optional<EventTypeOption> EventTypeOptionFromString(const std::string& str)
    {
        if (str == "bind") {
            return EventTypeOption::BIND;
        }
        else if (str == "unbind") {
            return EventTypeOption::UNBIND;
        }
        else if (str == "post") {
            return EventTypeOption::POST;
        }
        else {
            return std::nullopt;
        }
    }

    std::string EventTypeOptionToString(EventTypeOption opt)
    {
        switch (opt) {
        case EventTypeOption::BIND: return "bind";
        case EventTypeOption::POST: return "post";
        case EventTypeOption::UNBIND: return "unbind";
        default: return "unknown";
        }
    }

    FailureMode FailureMode::FromJson(const Json::Value& j)
    {
        FailureMode mode;
        mode.component = j["component"].asString();
        mode.version = j["version"].asString();
        mode.isMultiline = j["is_multiline"].asBool();
        mode.manifest = j["manifest"].asString();
        std::vector<PathCell> pathCells = {
            { std::nullopt, j["log_path"].asString() }
        };
        DataSourceOption opt;
        if (mode.component == "hardware" || mode.component == "urmacore" || mode.component == "udmacore") {
            opt = DataSourceOption::KERNEL;
        }
        else {
            opt = DataSourceOption::USER;
        }
        mode.dataSource = { opt, std::move(pathCells) };
        return mode;
    }

    Json::Value FailureEvent::ToJson() const
    {
        Json::Value j(Json::objectValue);
        if (auto time = TimestampToDatetimeStr(timestamp)) {
            j["time"] = *time;
        }
        else {
            j["time"] = Json::nullValue;
        }
        j["component"] = component;
        j["path"] = pathCell.path;
        j["text"] = text;
        j["attributes"] = Json::Value(Json::objectValue);
        if (attributes.find("function_name") != attributes.end()) {
            j["attributes"]["function_name"] = attributes.at("function_name");
        }
        if (attributes.find("local_eid") != attributes.end()) {
            j["attributes"]["local_eid"] = attributes.at("local_eid");
        }
        if (attributes.find("local_jetty_id") != attributes.end()) {
            j["attributes"]["local_jetty_id"] = attributes.at("local_jetty_id");
        }
        if (attributes.find("remote_eid") != attributes.end()) {
            j["attributes"]["remote_eid"] = attributes.at("remote_eid");
        }
        if (attributes.find("remote_jetty_id") != attributes.end()) {
            j["attributes"]["remote_jetty_id"] = attributes.at("remote_jetty_id");
        }
        return j;
    }

    Json::Value FailureMetadata::ToJson() const
    {
        Json::Value j(Json::objectValue);
        j["event_type"] = EventTypeOptionToString(eventType);
        if (podId) {
            j["pod_id"] = *podId;
        }
        else {
            j["pod_id"] = Json::nullValue;
        }
        j["program_name"] = programName;
        j["proc_id"] = procId;
        if (auto time = TimestampToDatetimeStr(timestamp)) {
            j["time"] = *time;
        }
        else {
            j["time"] = Json::nullValue;
        }
        j["local_eid"] = localEid;
        j["local_jetty_id"] = localJettyId;
        if (remoteEid) {
            j["remote_eid"] = *remoteEid;
        }
        else {
            j["remote_eid"] = Json::nullValue;
        }
        if (remoteJettyId) {
            j["remote_jetty_id"] = *remoteJettyId;
        }
        else {
            j["remote_jetty_id"] = Json::nullValue;
        }
        j["text"] = text;
        if (role) {
            j["role"] = *role;
        }
        else {
            j["role"] = Json::nullValue;
        }
        j["logs"] = Json::Value(Json::arrayValue);
        for (const FailureEvent* event : events) {
            if (event == nullptr) {
                continue;
            }
            j["logs"].append(event->ToJson());
        }
        return j;
    }

    PathCell::PathCell(const std::optional<std::string>& podId, const std::string& path)
        : podId(podId)
        , path(path)
    {
    }

    void Split(std::vector<std::string>& out, const std::string& str, char delim, bool keepEmpty)
    {
        out.clear();
        out.reserve(8);
        std::size_t i = 0;
        while (true) {
            size_t j = str.find(delim, i);
            if (j == std::string::npos) {
                auto part = str.substr(i);
                if (keepEmpty || !part.empty()) {
                    out.emplace_back(part);
                }
                break;
            }
            auto part = str.substr(i, j - i);
            if (keepEmpty || !part.empty()) {
                out.emplace_back(part);
            }
            i = j + 1;
        }
    }

    std::optional<int64_t> DatetimeStrToTimestamp(const std::string& datetimeStr)
    {
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        auto validate_and_convert = [&](std::tm& t, int64_t us, bool is_utc) -> std::optional<int64_t> {
            std::tm orginal = t;
            std::time_t s = is_utc ? timegm(&t) : mktime(&t);
            if (s == -1) {
                return std::nullopt;
            }

            std::tm normalized = { 0 };
            is_utc ? gmtime_r(&s, &normalized) : localtime_r(&s, &normalized);
            if (normalized.tm_year != orginal.tm_year || normalized.tm_mon != orginal.tm_mon || normalized.tm_mday != orginal.tm_mday ||
                normalized.tm_hour != orginal.tm_hour || normalized.tm_min != orginal.tm_min || normalized.tm_sec != orginal.tm_sec) {
                return std::nullopt;
            }

            int64_t total_us = static_cast<int64_t>(s) * 1000000 + us;
            if (total_us < 0 || total_us > now_us) {
                return std::nullopt;
            }

            return total_us;
            };

        std::tm t;

        std::memset(&t, 0, sizeof(t));
        t.tm_isdst = -1;
        const char* res = strptime(datetimeStr.c_str(), "[%a %b %d %H:%M:%S %Y]", &t);
        if (res && *res == '\0') {
            return validate_and_convert(t, 0, false);
        }

        std::memset(&t, 0, sizeof(t));
        t.tm_isdst = -1;
        res = strptime(datetimeStr.c_str(), "%Y-%m-%d %H:%M:%S", &t);
        if (res && *res == '\0') {
            if (datetimeStr[10] != ' ') {
                return std::nullopt;
            }
            return validate_and_convert(t, 0, false);
        }

        std::memset(&t, 0, sizeof(t));
        t.tm_isdst = -1;
        res = strptime(datetimeStr.c_str(), "%Y-%m-%dT%H:%M:%S", &t);
        if (res) {
            int64_t microseconds = 0;
            if (*res == '.') {
                res++;
                const char* end = res;
                while (std::isdigit(*end)) {
                    end++;
                }
                int len = end - res;
                if (len > 0) {
                    std::string fracStr(res, (len > 6 ? 6 : len));
                    if (len < 6) fracStr.append(6 - len, '0');
                    microseconds = std::stoll(fracStr);
                }
                res = end;
            }
            const char* tz = strptime(res, "%z", &t);
            if (tz && *tz == '\0') {
                return validate_and_convert(t, microseconds, true);
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> TimestampToDatetimeStr(int64_t timestamp, const std::string& format)
    {
        std::time_t seconds = static_cast<std::time_t>(timestamp / 1000000);
        std::tm t = { 0 };
        localtime_r(&seconds, &t);

        std::array<char, BUFFER_SIZE> buffer;
        if (format == "standard") {
            if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        else if (format == "iso8601") {
            if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%S.000000+08:00", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        else if (format == "syslog") {
            if (std::strftime(buffer.data(), buffer.size(), "[%a %b %d %H:%M:%S %Y]", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        return std::nullopt;
    }
}
