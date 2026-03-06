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

#include <atomic>
#include <fstream>
#include <mutex>
#include <thread>
#ifndef TOOL_MODE
#include <chrono>
#endif

#include "rack_module.h"
#include "log_reader.h"

namespace failure::log {
    class LogCollector final {
    public:
        RackResult Initialize();
        void UnInitialize();
        RackResult Start();
        void Stop();
    
    private:
        RackResult CreateReaders();
        RackResult ParseArg();
        RackResult ParsePodMode(const std::unordered_map<std::string, std::string>& argMap);
        RackResult ParseIOPath();
        RackResult ParseLogPath(const std::unordered_map<std::string, std::string>& argMap);
        RackResult ParseQueryCondition(const std::unordered_map<std::string, std::string>& argMap);
        void Save();

    private:
        bool podMode_{ false };
        std::string input_;
        std::string output_;
        std::unordered_map<std::string, std::vector<std::string>> customizedLogPath_;
        FailureEventQuery query_;

        std::atomic_bool running_{ false };
        std::vector<std::thread> workerThreads_;
        std::ofstream ofs_;
        std::mutex mutex_;

        std::vector<std::shared_ptr<LogReader>> readers_;
        std::vector<FailureEvent> events_;

#ifndef TOOL_MODE
        std::chrono::milliseconds interval_{ 1000 };
#endif
    };
}