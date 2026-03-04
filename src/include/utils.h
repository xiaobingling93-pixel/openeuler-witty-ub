#pragma once

#include <optional>
#include <string>
#include <vector>

namespace utils {
    void Split(std::vector<std::string> &out, const std::string &str, char delim, bool keepEmpty = false);
    std::optional<int64_t> DatetimeStrToTimestamp(const std::string &datetimeStr);
    std::optional<std::string> TimestampToDatetimeStr(int64_t timestamp,const std::string &format = "local");
}
