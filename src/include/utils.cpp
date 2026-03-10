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

#include "utils.h"

#include <array>
#include <chrono>
#include <ctime>
#include <cstring>

namespace utils {
    constexpr std::size_t BUFFER_SIZE = 64;

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