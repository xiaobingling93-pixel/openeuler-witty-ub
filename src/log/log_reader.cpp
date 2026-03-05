#define MODULE_NAME "LOG"

#include "log_reader.h"

#include <array>

#include "logger.h"
#include "utils.h"

namespace failure::log {
    constexpr std::size_t BUFFER_SIZE = 512;

    LogReader::LogReader(DataSourceOption option, const std::string& path, int64_t startTime, int64_t endTime)
        : path_(path)
        , startTime_(startTime)
        , endTime_(endTime)
        , handle_(nullptr)
        , parser_(std::make_unique<LogParser>())
    {
        ConfigureHandle(option);
    }

    LogReader::~LogReader()
    {
        DestroyHandle();
    }

    void LogReader::CreateHandle()
    {
        if (!handle_ && opener_) {
            handle_ = opener_(path_);
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
    }

    std::optional<FailureEvent> LogReader::ReadOnce()
    {
        while (true) {
            auto line = ReadNextLine();
            if (!line) {
                return std::nullopt;
            }

            if (auto entry = parser_->MatchSingleLineTemplate(*line)) {
                LogTemplate& tmpl = entry->first;
                std::unordered_map<std::string, std::string>& attributes = entry->second;
                if (auto event = tmpl.CreateEvent(attributes, *line)) {
                    event->path = path_;
                    return event;
                }
            }
            else if (auto entry = parser_->MatchMultiLineTemplate(*line)) {
                LogTemplate& tmpl = entry->first;
                std::unordered_map<std::string, std::string>& attributes = entry->second;
                auto it = attributes.find("identifier");
                std::string identifier = it->second;
                std::string lines;
                lines += *line;
                while (auto nextLine = ReadNextLine()) {
                    auto nextAttributes = tmpl.Match(*nextLine);
                    if (nextAttributes &&
                        nextAttributes->find("identifier") != nextAttributes->end() &&
                        nextAttributes->at("identifier") == identifier) {
                        lines += *nextLine;
                        continue;
                    }
                    cachedLine_ = std::move(nextLine);
                    break;
                }
                attributes.erase(it);
                if (auto event = tmpl.CreateEvent(attributes, lines)) {
                    event->path = path_;
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

        std::array<char, BUFFER_SIZE> buffer;
        if (!fgets(buffer.data(), buffer.size(), handle_)) {
            return std::nullopt;
        }

        return std::string(buffer.data());
    }

    void LogReader::ConfigureHandle(DataSourceOption option)
    {
        if (option == DataSourceOption::KERNEL) {
            opener_ = [&](const std::string& p) {
                std::string awkScript = R"(BEGIN{
  mon["Jan"]=1; mon["Feb"]=2; mon["Mar"]=3; mon["Apr"]=4;
  mon["May"]=5; mon["Jun"]=6; mon["Jul"]=7; mon["Aug"]=8;
  mon["Sep"]=9; mon["Oct"]=10; mon["Nov"]=11; mon["Dec"]=12;
}
{
  match($0, /^\[(.*)\]/, a)
  split(a[1], f, " ")
  split(f[4], t, ":")
  ts = mktime(f[5]" "mon[f[2]]" "f[3]" "t[1]" "t[2]" "t[3]")
  if (ts < s) next
  if (ts > e) exit
  print
})";
                std::string cmd = p + " -T | gawk -v s=" + std::to_string(startTime_) + " -v e=" + std::to_string(endTime_) + " '" + awkScript + "'";
                return popen(cmd.c_str(), "r");
                };
            closer_ = [](FILE* f) {
                pclose(f);
                };
        }
        else {
            opener_ = [&](const std::string& p) {
                auto startTimeStr = utils::TimestampToDatetimeStr(startTime_, "iso8601");
                auto endTimeStr = utils::TimestampToDatetimeStr(endTime_, "iso8601");
                std::string cmd = "awk -F'|' '$1 >= \"" + *startTimeStr + ".000000+08:00\" && $1 <= \"" + *endTimeStr + ".999999+08:00\"' " + p;
                return popen(cmd.c_str(), "r");
                };
            closer_ = [](FILE* f) {
                pclose(f);
                };
        }
    }
}