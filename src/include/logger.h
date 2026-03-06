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

#pragma once
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/appender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <memory>

#ifndef MODULE_NAME
    #define MODULE_NAME "UNKNOWN"
#endif

namespace rack::logger {
    // 全局日志配置标志
    inline bool g_debug_enabled = false;
    
    // 全局 Logger 实例
    inline log4cplus::Logger g_logger;
    
    inline void init(const char* argv0) {
        // 检查是否启用调试日志
        const char* debug_env = std::getenv("RACK_DEBUG");
        g_debug_enabled = (debug_env && std::string(debug_env) == "1");
        
        // 初始化 log4cplus
        log4cplus::initialize();
        
        // 获取 root logger
        g_logger = log4cplus::Logger::getRoot();
        
        // 创建控制台输出器
        log4cplus::SharedAppenderPtr console_appender(new log4cplus::ConsoleAppender());
        
        // 创建布局格式（简单格式，因为我们会在 LogStream 中添加自定义前缀）
        console_appender->setLayout(std::make_unique<log4cplus::PatternLayout>("%m%n"));
        
        // 添加输出器到 logger
        g_logger.addAppender(console_appender);
        
        // 设置日志级别
        if (g_debug_enabled) {
            g_logger.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
        } else {
            g_logger.setLogLevel(log4cplus::INFO_LOG_LEVEL);
        }
        
        // argv0 参数保留以保持接口兼容性，但不使用
        (void)argv0;
    }
    
    inline void shutdown() {
        log4cplus::Logger::shutdown();
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
    
    class LogStream {
    public:
        enum Level { INFO, WARN, ERROR };
        
        LogStream(Level level, const char* module) 
            : level_(level), module_(module), timestamp_(current_timestamp()) {}
        
        ~LogStream() {
            if (!ss_.str().empty()) {
                std::string prefix;
                switch (level_) {
                    case INFO:
                        prefix = "[INFO][" + timestamp_ + "][" + std::string(module_) + "] ";
                        break;
                    case WARN:
                        prefix = "[WARN][" + timestamp_ + "][" + std::string(module_) + "] ";
                        break;
                    case ERROR:
                        prefix = "[ERROR][" + timestamp_ + "][" + std::string(module_) + "] ";
                        break;
                }
                
                std::string message = prefix + ss_.str();
                
                switch (level_) {
                    case INFO:
                        LOG4CPLUS_INFO(g_logger, message);
                        break;
                    case WARN:
                        LOG4CPLUS_WARN(g_logger, message);
                        break;
                    case ERROR:
                        LOG4CPLUS_ERROR(g_logger, message);
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
    
    class DebugLogStream {
    public:
        DebugLogStream(const char* file, int line, const char* module) 
            : file_(file), line_(line), module_(module), timestamp_(current_timestamp()) {}
        
        ~DebugLogStream() {
            if (!ss_.str().empty() && g_debug_enabled) {
                std::string prefix = "[DEBUG][" + timestamp_ + "][" + std::string(module_) + "][" + 
                                   std::string(file_) + ":" + std::to_string(line_) + "] ";
                std::string message = prefix + ss_.str();
                LOG4CPLUS_DEBUG(g_logger, message);
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

// 保持宏定义不变，确保对外接口完全兼容
#define LOG_INFO rack::logger::LogStream(rack::logger::LogStream::INFO, MODULE_NAME)
#define LOG_WARN rack::logger::LogStream(rack::logger::LogStream::WARN, MODULE_NAME)
#define LOG_ERROR rack::logger::LogStream(rack::logger::LogStream::ERROR, MODULE_NAME)
#define LOG_DEBUG if (rack::logger::g_debug_enabled) rack::logger::DebugLogStream(__FILE__, __LINE__, MODULE_NAME)


