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
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>

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
        RackResult InitIO();
        RackResult ParseArgs();
        RackResult ParsePodMode(const std::unordered_map<std::string, std::string>& argMap);
        RackResult ParseLogPath(const std::unordered_map<std::string, std::string>& argMap);
        RackResult ParseQueryCondition(const std::unordered_map<std::string, std::string>& argMap);
        RackResult CreateReaders();
        void CollectUmqMetadata(
            std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap,
            std::vector<FailureMetadata>& localMetadata
        );
        void CollectCorrelatedLogs(
            std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap,
            std::vector<FailureMetadata>& localMetadata
        );
        RackResult CorrelateEvents(std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap);
        void Save();

        bool IsValidPodId(const std::string& podId) const;
        bool IsValidPath(const std::string& p) const;
        bool IsValidEid(const std::string& eid) const;
        bool IsValidJettyId(const std::string& jettyId) const;

    private:
        bool podMode_{ false };
        std::unordered_map<std::string, std::vector<PathCell>> customizedLogPath_;
        std::unordered_map<std::string, std::unordered_set<std::string>> allowedPodIds_;
        FailureEventQuery query_;

        std::vector<std::thread> workerThreads_;

        std::vector<std::shared_ptr<LogReader>> readers_;
        std::vector<FailureMetadata> metadata_;

        std::mutex stateMutex_;
        std::condition_variable stateCond_;
        bool startInProgress_{ false };
        std::atomic_bool readerActive_{ false };
        std::mutex metadataMutex_;
        std::mutex ofsMutex_;
    };
}
