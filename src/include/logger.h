#pragma once
#include <glog/logging.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

#ifndef MODULE_NAME
    #define MODULE_NAME "UNKNOWN"
#endif

namespace rack::logger {
    inline void init(const char* argv0) {
        google::InitGoogleLogging(argv0);
        FLAGS_log_prefix = false;
        FLAGS_logtostderr = true;
        FLAGS_colorlogtostderr = true;

        const char* debug_env = std::getenv("RACK_DEBUG");
        FLAGS_v = (debug_env && std::string(debug_env) == "1") ? 1 : 0;
    }
    inline void shutdown() {
        google::ShutdownGoogleLogging();
    }
    inline std::string current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    class LogStream{
        public:
            enum Level{INFO, WARN, ERROR};
            LogStream(Level level,const char* module):level_(level),module_(module),timestamp_(current_timestamp()) {}
            ~LogStream() {
                if (!ss_.str().empty()) {
                    std::string prefix;
                    switch (level_) {
                        case INFO:
                            prefix = "[INFO]["+timestamp_+"]["+std::string(module_)+"] ";
                            break;
                        case WARN:
                            prefix = "[WARN]["+timestamp_+"]["+std::string(module_)+"] ";
                            break;
                        case ERROR:
                            prefix = "[ERROR]["+timestamp_+"]["+std::string(module_)+"] ";
                            break;
                    }
                    switch (level_) {
                        case INFO:
                            LOG(INFO) << prefix << ss_.str();
                            break;
                        case WARN:
                            LOG(WARNING) << prefix << ss_.str();
                            break;
                        case ERROR:
                            LOG(ERROR) << prefix << ss_.str();
                            break;
                    }
                }
            }
            template <typename T>
            LogStream& operator<<(const T& value) {
                ss_ << value;
                return *this;
            }

        private:
            Level level_;
            const char* module_;
            std::string timestamp_;
            std::stringstream ss_;
    };
    class DebugLogStream{
        public:
            DebugLogStream(const char* file,int line,const char* module) : file_(file),line_(line),module_(module) {}
            ~DebugLogStream() {
                if (!ss_.str().empty() && FLAGS_v >= 1) {
                    std::string prefix = "[DEBUG]["+timestamp_+"]["+std::string(module_)+"]["+std::string(file_)+":"+std::to_string(line_)+"] ";
                    LOG(INFO) << prefix << ss_.str();
                }
            }
            template <typename T>
            DebugLogStream& operator<<(const T& value) {
                ss_ << value;
                return *this;
            }
        private:
            const char* file_;
            int line_;
            const char* module_;
            std::string timestamp_;
            std::stringstream ss_;
    };
} // namespace rack::logger

#define LOG_INFO rack::logger::LogStream(rack::logger::LogStream::INFO, MODULE_NAME)
#define LOG_WARN rack::logger::LogStream(rack::logger::LogStream::WARN, MODULE_NAME)
#define LOG_ERROR rack::logger::LogStream(rack::logger::LogStream::ERROR, MODULE_NAME)
#define LOG_DEBUG if (FLAGS_v >= 1) rack::logger::DebugLogStream(__FILE__, __LINE__, MODULE_NAME)