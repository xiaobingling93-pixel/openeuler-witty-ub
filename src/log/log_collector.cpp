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

#include "log_collector.h"

#include <filesystem>

#include "failure_def.h"
#include "logger.h"
#include "ubse_context.h"
#include "utils.h"

namespace failure::log {
    using namespace ubse::context;

    RackResult LogCollector::Initialize()
    {
        auto res = ParseArg();
        if (res != RACK_OK) {
            LOG_ERROR << "failed to create log readers";
            return res;
        }
        res = CreateReaders();
        if (res != RACK_OK) {
            LOG_ERROR << "failed to create log readers";
            return res;
        }
        return res;
    }

    void LogCollector::UnInitialize()
    {
    }

    RackResult LogCollector::Start()
    {
        auto ReaderLoopOnce = [&](auto& reader) {
            try {
                reader->CreateHandle();
            }
            catch (const std::exception& e) {
                LOG_ERROR << "failed to create handle: " << e.what();
                return;
            }

            while (running_.load(std::memory_order_acquire)) {
                auto event = reader->ReadOnce();
                if (!event) {
                    break;
                }
                if (!query_.Match(*event)) {
                    continue;
                }
                {
                    std::lock_guard<std::mutex> lk(mutex_);
                    events_.push_back(std::move(*event));
                }
            }

            reader->DestroyHandle();
            };

        if (running_.load(std::memory_order_acquire)) {
            return RACK_OK;
        }
        running_.store(true, std::memory_order_release);

        workerThreads_.clear();
        workerThreads_.reserve(readers_.size());

#ifndef TOOL_MODE
        LOG_DEBUG << "LogCollector running with per-reader threads (poll)";
        for (auto reader : readers_) {
            workerThreads_.emplace_back([&, reader]() {
                while (running_.load(std::memory_order_acquire)) {
                    ReaderLoopOnce(reader);
                    std::this_thread::sleep_for(interval_);
                }
            });
        }
#else
        LOG_DEBUG << "LogCollector running in tool mode with per-reader threads (run once)";
        for (auto reader : readers_) {
            workerThreads_.emplace_back([&,reader]() {
                ReaderLoopOnce(reader);
                });
        }
        for (auto& t : workerThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        workerThreads_.clear();

        running_.store(false, std::memory_order_release);
#endif

        return RACK_OK;
    }

    void LogCollector::Stop()
    {
#ifndef TOOL_MODE
        running_.store(false, std::memory_order_release);
        for (auto& t : workerThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        workerThreads_.clear();
#endif
        if (ofs_.is_open()) {
            ofs_.flush();
            ofs_.close();
        }
    }

    RackResult LogCollector::CreateReaders()
    {
        std::ifstream ifs(input_);
        if (!ifs.is_open()) {
            LOG_ERROR << "failed to open input file: " << input_;
            return RACK_FAIL;
        }
        
        // 读取整个文件内容
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        ifs.close();
        
        // 去除空白字符
        content.erase(std::remove_if(content.begin(), content.end(), 
                     [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }), 
                     content.end());
        
        // 检查是否是数组
        if (content.empty() || content[0] != '[' || content.back() != ']') {
            LOG_ERROR << "failure mode file must be an array";
            return RACK_FAIL;
        }
        
        // 提取数组内容
        std::string arrayContent = content.substr(1, content.length() - 2);
        
        // 解析数组中的每个对象
        std::vector<std::string> items;
        int depth = 0;
        size_t start = 0;
        
        for (size_t i = 0; i < arrayContent.size(); i++) {
            if (arrayContent[i] == '{') {
                if (depth == 0) {
                    start = i;
                }
                depth++;
            } else if (arrayContent[i] == '}') {
                depth--;
                if (depth == 0) {
                    items.push_back(arrayContent.substr(start, i - start + 1));
                }
            }
        }

        std::unordered_map<std::string, std::shared_ptr<LogReader>> dataSourceReaderMap;
        for (const std::string& itemStr : items) {
            FailureMode mode = FailureMode::FromJson(itemStr);
            if (mode.dataSource.option == DataSourceOption::UNKNOWN) {
                continue;
            }
            for (const std::string& path : mode.dataSource.paths) {
                std::filesystem::path p = path;
                std::vector<std::string> logCells;
                if (mode.dataSource.option == DataSourceOption::USER) {
                    if (!std::filesystem::exists(p)) {
                        LOG_WARN << "file not found: " << p;
                        continue;
                    }
                    if (std::filesystem::is_regular_file(p)) {
                        logCells.push_back(path);
                    }
                    else if (std::filesystem::is_directory(p)) {
                        for (const auto& entry : std::filesystem::directory_iterator(p)) {
                            if (entry.path().extension() == ".log") {
                                logCells.push_back(entry.path().string());
                            }
                        }
                    }
                }
                else {
                    logCells.push_back(path);
                }
                for (const std::string& logCell : logCells) {
                    auto it = dataSourceReaderMap.find(logCell);
                    if (it == dataSourceReaderMap.end()) {
                        auto reader = std::make_shared<LogReader>(mode.dataSource.option, logCell, query_.startTime, query_.endTime);
                        it = dataSourceReaderMap.emplace(logCell, std::move(reader)).first;
                    }
                    it->second->AddFailureMode(mode);
                }
            }
        }

        readers_.reserve(dataSourceReaderMap.size());
        for (auto& [_, reader] : dataSourceReaderMap) {
            readers_.push_back(std::move(reader));
        }
        return RACK_OK;
    }

    RackResult LogCollector::ParseArg()
    {
        RackResult ret = RACK_OK;
        auto& argMap = UbseContext::GetInstance().GetArgMap();
        if ((ret = ParsePodMode(argMap)) != RACK_OK) {
            return ret;
        }
        if ((ret = ParseIOPath()) != RACK_OK) {
            return ret;
        }
        if ((ret = ParseLogPath(argMap)) != RACK_OK) {
            return ret;
        }
        if ((ret = ParseQueryCondition(argMap)) != RACK_OK) {
            return ret;
        }
        return RACK_OK;
    }

    RackResult LogCollector::ParsePodMode(const std::unordered_map<std::string, std::string>& argMap)
    {
        auto it = argMap.find("pod_mode");
        if (it == argMap.end()) {
            LOG_ERROR << "missing argument pod_mode";
            return RACK_FAIL;
        }
        if (it->second == "on") {
            podMode_ = true;
        }
        return RACK_OK;
    }

    RackResult LogCollector::ParseIOPath()
    {
        // TODO: 支持可配置
        input_ = "../data/failure_mode.json";
        output_ = "../data/failure_event.json";

        std::filesystem::path outFile = output_;
        if (std::filesystem::exists(outFile)) {
            std::filesystem::remove(outFile);
        }
        ofs_.open(output_, std::ios::app);
        if (!ofs_.is_open()) {
            LOG_ERROR << "failed to open output file: " << output_;
            return RACK_FAIL;
        }
        static constexpr size_t kBufSize = 1 << 20;
        static char buf[kBufSize];
        ofs_.rdbuf()->pubsetbuf(buf, kBufSize);
        std::ios::sync_with_stdio(false);

        return RACK_OK;
    }

    RackResult LogCollector::ParseLogPath(const std::unordered_map<std::string, std::string>& argMap)
    {
        auto ComponentOf = [](const std::string& arg) -> std::string {
            auto p = arg.find('_');
            return (p == std::string::npos) ? arg : arg.substr(0, p);
            };

        auto HandleLogPath = [&](const std::string& arg, bool podRequired, bool podSplitAndStrip) -> RackResult {
            const std::string component = ComponentOf(arg);
            auto it = argMap.find(arg);
            if (podMode_) {
                if (podRequired && it == argMap.end()) {
                    LOG_ERROR << "missing argument " << arg << ", required in pod mode";
                    return RACK_FAIL;
                }
                if (it == argMap.end()) {
                    return RACK_OK;
                }
                if (!podSplitAndStrip) {
                    customizedLogPath_[component].push_back(it->second);
                    return RACK_OK;
                }
                std::vector<std::string> items;
                utils::Split(items, it->second, ',');
                for (const auto& s : items) {
                    auto pos = s.find(':');
                    customizedLogPath_[component].push_back(s.substr(pos + 1));
                }
                return RACK_OK;
            }
            if (it != argMap.end()) {
                customizedLogPath_[component].push_back(it -> second);
            }
            return RACK_OK;
            };

        if (HandleLogPath("ubsocket_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("umq_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("liburma_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("urmacore_log_path", /*podRequired=*/false, /*podSplitAndStrip=*/false) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("libudma_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("udmacore_log_path", /*podRequired=*/false, /*podSplitAndStrip=*/false) != RACK_OK) return RACK_FAIL;
    
        return RACK_OK;
    }

    RackResult LogCollector::ParseQueryCondition(const std::unordered_map<std::string, std::string>& argMap)
    {
        std::unordered_map<std::string, std::string>::const_iterator it;

        if ((it = argMap.find("start_time")) == argMap.end()) {
            LOG_ERROR << "missing argument start_time";
            return RACK_FAIL;
        }
        auto startTime = utils::DatetimeStrToTimestamp(it->second);
        if (!startTime) {
            LOG_ERROR << "invalid argument start_time: " << it->second;
            return RACK_FAIL;
        }
        query_.startTime = *startTime;

        if ((it = argMap.find("end_time")) == argMap.end()) {
            LOG_ERROR << "missing argument end_time";
            return RACK_FAIL;
        }
        auto endTime = utils::DatetimeStrToTimestamp(it->second);
        if (!endTime) {
            LOG_ERROR << "invalid argument end_time: " << it->second;
            return RACK_FAIL;
        }
        query_.endTime = *endTime;

        if ((it = argMap.find("event_type")) != argMap.end()) {
            std::vector<std::string> eventTypes;
            utils::Split(eventTypes, it->second, ',');
            for (const std::string& eventType : eventTypes) {
                query_.eventTypes.push_back(EventTypeOptionFromString(eventType));
            }
        }
        if ((it = argMap.find("pod_id")) != argMap.end()) {
            std::vector<std::string> podIds;
            utils::Split(podIds, it->second, ',');
            for (const std::string& podId : podIds) {
                query_.podIds.push_back(podId);
            }
        }
        if ((it = argMap.find("local_eid")) != argMap.end()) {
            std::vector<std::string> localEids;
            utils::Split(localEids, it->second, ',');
            for (const std::string& localEid : localEids) {
                query_.localEids.push_back(localEid);
            }
        }
        if ((it = argMap.find("jetty_id")) != argMap.end()) {
            std::vector<std::string> jettyIds;
            utils::Split(jettyIds, it->second, ',');
            for (const std::string& jettyId : jettyIds) {
                query_.jettyIds.push_back(jettyId);
            }
        }

        return RACK_OK;
    }

    void LogCollector::Save()
    {
        ofs_ << "[\n";
        bool first = true;
        for (const FailureEvent& event : events_) {
            if (!first) {
                ofs_ << ",\n";
            }
            ofs_ << "    " << event.ToJson();
            first = false;
        }
        ofs_ << "\n]";
    }
}