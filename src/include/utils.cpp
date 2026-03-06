#include "utils.h"

#include <array>
#include <ctime>

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
        std::tm t = { 0 };
        t.tm_isdst = -1;
        const char* res = nullptr;
        res = strptime(datetimeStr.c_str(), "[%a %b %d %H:%M:%S %Y]", &t);
        if (res && *res == '\0') {
            std::time_t s = mktime(&t);
            if (s == -1) {
                return std::nullopt;
            }
            return static_cast<int64_t>(s) * 1000000;
        }
        res = strptime(datetimeStr.c_str(), "%Y-%m-%d %H:%M:%S", &t);
        if (res && *res == '\0') {
            std::time_t s = mktime(&t);
            if (s == -1) {
                return std::nullopt;
            }
            return static_cast<int64_t>(s) * 1000000;
        }
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
                    std::string fracStr(res, len);
                    if (len > 6) {
                        fracStr = fracStr.substr(0, 6);
                    }
                    else {
                        fracStr.append(6 - len, '0');
                    }
                    microseconds = std::stoll(fracStr);
                }
                res = end;
            }
            const char* tz = strptime(res, "%z", &t);
            if (tz) {
                std::time_t s = timegm(&t);
                if (s == -1) {
                    return std::nullopt;
                }
                return static_cast<int64_t>(s) * 1000000 + microseconds;
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
        if (format == "local") {
            if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        else if (format == "iso8601") {
            if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        else if (format == "syslog") {
            if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &t) > 0) {
                return std::string(buffer.data());
            }
        }
        return std::nullopt;
    }
}