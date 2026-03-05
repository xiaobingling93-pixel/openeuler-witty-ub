#include <vector>

#include "failure_def.h"
#include "log_parser.h"

namespace failure::log {
    class LogReader {
    public:
        LogReader(DataSourceOption option, const std::string& path, int64_t startTime, int64_t endTime);
        ~LogReader();

        void CreateHandle();
        void DestroyHandle();
        void AddFailureMode(const FailureMode& mode);
        std::optional<FailureEvent> ReadOnce();
    
    private:
        std::optional<std::string> ReadNextLine();
        void ConfigureHandle(DataSourceOption option);
    
    private:
        std::string path_;
        int64_t startTime_;
        int64_t endTime_;

        FILE* handle_;
        std::function<FILE* (const std::string&)> opener_;
        std::function<void(FILE*)> closer_;

        std::optional<std::string> cachedLine_;
        std::unique_ptr<LogParser> parser_;
    };
}