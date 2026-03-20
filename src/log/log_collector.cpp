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

#include <algorithm>
#include <filesystem>

#include <re2/re2.h>

#include "failure_def.h"
#include "logger.h"
#include "ubse_context.h"

namespace failure::log {
    using namespace ubse::context;

    constexpr const char* FAILURE_MODE_FILE = "/usr/share/witty-ub/data/failure-mode.json";
    constexpr const char* FAILURE_MODE_FILE_DEV = "./data/failure-mode.json";
    constexpr const char* FAILURE_EVENT_FILE = "/var/witty-ub/failure-event.json";
    constexpr const int64_t TIME_WINDOW_US = 10 * 1000000LL;
    constexpr const int LOCAL_EID_IDX = 1;
    constexpr const int LOCAL_JETTY_ID_IDX = 2;
    constexpr const int REMOTE_EID_IDX = 3;
    constexpr const int REMOTE_JETTY_ID_IDX = 4;

    inline static const re2::RE2 biEndRe(
        R"(local eid: ([0-9a-f:]+), local jetty_id: (\d+), remote eid: ([0-9a-f:]+), remote jetty_id: (\d+))"
    );
    inline static const re2::RE2 singleEndRe(
        R"(eid: ([0-9a-f:]+), jetty_id: (\d+))"
    );

    void CacheUmqEndpointFields(FailureEvent& event)
    {
        auto localEidIt = event.attributes.find("local_eid");
        auto localJettyIdIt = event.attributes.find("local_jetty_id");
        if (localEidIt != event.attributes.end() && localJettyIdIt != event.attributes.end()) {
            return;
        }

        auto contentIt = event.attributes.find("content");
        if (contentIt == event.attributes.end()) {
            return;
        }

        std::string localEid;
        std::string localJettyId;
        std::string remoteEid;
        std::string remoteJettyId;
        if (re2::RE2::PartialMatch(
            contentIt->second,
            biEndRe,
            &localEid,
            &localJettyId,
            &remoteEid,
            &remoteJettyId)
        ) {
            event.attributes["local_eid"] = std::move(localEid);
            event.attributes["local_jetty_id"] = std::move(localJettyId);
            event.attributes["remote_eid"] = std::move(remoteEid);
            event.attributes["remote_jetty_id"] = std::move(remoteJettyId);
        } else if (re2::RE2::PartialMatch(contentIt->second, singleEndRe, &localEid, &localJettyId)) {
            event.attributes["local_eid"] = std::move(localEid);
            event.attributes["local_jetty_id"] = std::move(localJettyId);
        }
    }

    RackResult LogCollector::Initialize()
    {
        RackResult res;
        if ((res = InitIO()) != RACK_OK) {
            LOG_ERROR << "failed to init io";
            return res;
        }
        if ((res = ParseArgs()) != RACK_OK) {
            LOG_ERROR << "failed to parse args";
            return res;
        }
        if ((res = CreateReaders()) != RACK_OK) {
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
        std::unordered_map<std::string, std::vector<FailureEvent>> eventsMap;
        std::mutex eventsMapMutex;

        auto ReaderLoopOnce = [&](std::shared_ptr<LogReader> reader) {
            std::unordered_map<std::string, std::vector<FailureEvent>> localEventsMap;
            bool handleCreated = false;
            try {
                reader->CreateHandle();
                handleCreated = true;
                while (true) {
                    if (!readerActive_.load(std::memory_order_acquire)) {
                        break;
                    }
                    auto event = reader->ReadOnce();
                    if (!event) {
                        break;
                    }
                    localEventsMap[event->component].push_back(std::move(*event));
                }
            }
            catch (const std::exception& e) {
                LOG_ERROR << "reader loop failed: " << e.what();
                return;
            }

            if (handleCreated) {
                try {
                    reader->DestroyHandle();
                }
                catch (const std::exception& e) {
                    LOG_ERROR << "failed to destroy handle: " << e.what();
                }
            }

            {
                std::lock_guard<std::mutex> lk(eventsMapMutex);
                for (auto& [component, events] : localEventsMap) {
                    auto& dst = eventsMap[component];
                    dst.reserve(dst.size() + events.size());
                    dst.insert(dst.end(), std::make_move_iterator(events.begin()), std::make_move_iterator(events.end()));
                }
            }
            };

        {
            std::unique_lock<std::mutex> lk(stateMutex_);
            stateCond_.wait(lk, [this]() { return !startInProgress_; });
            startInProgress_ = true;
        }
        readerActive_.store(true, std::memory_order_release);

        {
            struct RunningGuard {
                LogCollector* self;
                ~RunningGuard()
                {
                    self->readerActive_.store(false, std::memory_order_release);
                    {
                        std::lock_guard<std::mutex> lk(self->stateMutex_);
                        self->startInProgress_ = false;
                    }
                    self->stateCond_.notify_one();
                }
            }guard{ this };
            {
                std::lock_guard<std::mutex> lk(metadataMutex_);
                metadata_.clear();
            }
            workerThreads_.clear();
            workerThreads_.reserve(readers_.size());

#ifdef ENABLE_TOOL_FEATURE
            LOG_DEBUG << "LogCollector running in tool mode with per-reader threads (run once)";
            for (auto reader : readers_) {
                workerThreads_.emplace_back([&, reader]() {
                    ReaderLoopOnce(reader);
                    });
            }
            for (auto& t : workerThreads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
            workerThreads_.clear();

            CorrelateEvents(eventsMap);
#endif
        }

#ifdef ENABLE_TOOL_FEATURE
        Save();
#endif

        return RACK_OK;
    }

    void LogCollector::Stop()
    {
        readerActive_.store(false, std::memory_order_release);
        for (auto& t : workerThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        workerThreads_.clear();
    }

    RackResult LogCollector::InitIO()
    {
        std::filesystem::path outFile = FAILURE_EVENT_FILE;
        std::error_code ec;
        std::filesystem::create_directories(outFile.parent_path(), ec);
        if (ec) {
            LOG_ERROR << "failed to prepare output directory: " << outFile.parent_path() << ", error: " << ec.message();
            return RACK_FAIL;
        }
        std::ios::sync_with_stdio(false);

        return RACK_OK;
    }

    RackResult LogCollector::ParseArgs()
    {
        RackResult ret = RACK_OK;
        auto& argMap = UbseContext::GetInstance().GetArgMap();
        if ((ret = ParsePodMode(argMap)) != RACK_OK) {
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
        auto it = argMap.find("pod-mode");
        if (it == argMap.end()) {
            LOG_ERROR << "missing argument pod-mode";
            return RACK_FAIL;
        }
        std::string_view podMode = it->second;
        if (podMode != "on" && podMode != "off") {
            LOG_ERROR << "invalid argument pod-mode: " << podMode << ", expected \"on\" or \"off\"";
            return RACK_FAIL;
        }
        podMode_ = podMode == "on";
        return RACK_OK;
    }

    RackResult LogCollector::ParseLogPath(const std::unordered_map<std::string, std::string>& argMap)
    {
        auto ComponentOf = [](const std::string& arg) -> std::string {
            auto p = arg.find('-');
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
                    if (!IsValidPath(it->second) != RACK_OK) {
                        LOG_ERROR << "invalid argument: " << arg;
                        return RACK_FAIL;
                    }
                    customizedLogPath_[component].emplace_back(std::nullopt, it->second);
                    return RACK_OK;
                }
                std::vector<std::string> entries;
                failure::Split(entries, it->second, ',');
                if (entries.empty()) {
                    LOG_ERROR << "invalid argument " << arg << ": " << it->second << ", expected \"<pod-id1>:<path1>,<pod-id2>:<path2>,...\"";
                    return RACK_FAIL;
                }
                for (const std::string& entry : entries) {
                    auto pos = entry.find(':');
                    if (pos == std::string::npos) {
                        LOG_ERROR << "invalid argument " << arg << ": " << entry << ", expected \"<pod-id>:<path>\"";
                        return RACK_FAIL;
                    }
                    const std::string& podId = entry.substr(0, pos);
                    if (allowedPodIds_.find(component) != allowedPodIds_.end() && allowedPodIds_[component].find(podId) != allowedPodIds_[component].end()) {
                        LOG_ERROR << "duplicated pod-id: " << podId << " for component " << component;
                        return RACK_FAIL;
                    }
                    const std::string& path = entry.substr(pos + 1);
                    if (!IsValidPodId(podId) || !IsValidPath(path)) {
                        LOG_ERROR << "invalid argument: " << arg;
                        return RACK_FAIL;
                    }
                    customizedLogPath_[component].emplace_back(podId, path);
                    allowedPodIds_[component].insert(podId);
                }
                return RACK_OK;
            }
            if (it != argMap.end()) {
                if (!IsValidPath(it->second) != RACK_OK) {
                    LOG_ERROR << "invalid argument: " << arg;
                    return RACK_FAIL;
                }
                customizedLogPath_[component].emplace_back(std::nullopt, it->second);
            }
            return RACK_OK;
            };
        if (HandleLogPath("ubsocket-log-path", true, true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("umq-log-path", true, true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("liburma-log-path", true, true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("urmacore-log-path", false, false) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("libudma-log-path", true, true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("udmacore-log-path", false, false) != RACK_OK) return RACK_FAIL;

        return RACK_OK;
    }

    RackResult LogCollector::ParseQueryCondition(const std::unordered_map<std::string, std::string>& argMap)
    {
        std::unordered_map<std::string, std::string>::const_iterator it;

        if ((it = argMap.find("start-time")) == argMap.end()) {
            LOG_ERROR << "missing argument start-time";
            return RACK_FAIL;
        }
        const std::string& startTimeStr = it->second;
        auto startTime = failure::DatetimeStrToTimestamp(startTimeStr);
        if (!startTime) {
            LOG_ERROR << "invalid argument start-time: " << startTimeStr;
            return RACK_FAIL;
        }
        query_.startTime = *startTime;
        if ((it = argMap.find("end-time")) == argMap.end()) {
            LOG_ERROR << "missing argument end-time";
            return RACK_FAIL;
        }
        const std::string& endTimeStr = it->second;
        auto endTime = failure::DatetimeStrToTimestamp(endTimeStr);
        if (!endTime) {
            LOG_ERROR << "invalid argument end-time: " << endTimeStr;
            return RACK_FAIL;
        }
        query_.endTime = *endTime + 999999LL;
        if (query_.startTime > query_.endTime) {
            LOG_ERROR << "logic error: start-time (" << startTimeStr << ") is later than end-time (" << endTimeStr << ")";
            return RACK_FAIL;
        }

        if ((it = argMap.find("event-type")) != argMap.end()) {
            std::vector<std::string> eventTypes;
            failure::Split(eventTypes, it->second, ',');
            if (eventTypes.empty()) {
                LOG_ERROR << "empty argument event-type";
                return RACK_FAIL;
            }
            for (const std::string& eventType : eventTypes) {
                auto eventTypeOpt = EventTypeOptionFromString(eventType);
                if (!eventTypeOpt) {
                    LOG_ERROR << "invalid argument event-type: " << eventType;
                    return RACK_FAIL;
                }
                query_.eventTypes.insert(*eventTypeOpt);
            }
        }
        if ((it = argMap.find("pod-id")) != argMap.end()) {
            if (!podMode_) {
                LOG_ERROR << "unexpected arg pod-id in non pod mode";
                return RACK_FAIL;
            }
            if (it->second.empty()) {
                LOG_ERROR << "empty argument pod-id";
                return RACK_FAIL;
            }
            std::vector<std::string> podIds;
            failure::Split(podIds, it->second, ',');
            for (const std::string& podId : podIds) {
                if (!IsValidPodId(podId)) {
                    return RACK_FAIL;
                }
                if (query_.podIds.find(podId) != query_.podIds.end()) {
                    LOG_ERROR << "duplicated pod-id: " << podId;
                    return RACK_FAIL;
                }
                for (const auto& [component, podIds] : allowedPodIds_) {
                    if (podIds.find(podId) == podIds.end()) {
                        LOG_ERROR << "pod-id: " << podId << ", not provided in log path";
                        return RACK_FAIL;
                    }
                }
                query_.podIds.insert(podId);
            }
        }
        if ((it = argMap.find("local-eid")) != argMap.end()) {
            std::vector<std::string> localEids;
            failure::Split(localEids, it->second, ',');
            for (const std::string& localEid : localEids) {
                if (!IsValidEid(localEid)) {
                    return RACK_FAIL;
                }
                if (query_.localEids.find(localEid) != query_.localEids.end()) {
                    LOG_ERROR << "duplicated local-eid: " << localEid;
                    return RACK_FAIL;
                }
                query_.localEids.insert(localEid);
            }
        }
        if ((it = argMap.find("jetty-id")) != argMap.end()) {
            std::vector<std::string> jettyIds;
            failure::Split(jettyIds, it->second, ',');
            for (const std::string& jettyId : jettyIds) {
                if (!IsValidJettyId(jettyId)) {
                    return RACK_FAIL;
                }
                if (query_.jettyIds.find(jettyId) != query_.jettyIds.end()) {
                    LOG_ERROR << "duplicated jetty-id: " << jettyId;
                    return RACK_FAIL;
                }
                query_.jettyIds.insert(jettyId);
            }
        }

        return RACK_OK;
    }

    RackResult LogCollector::CreateReaders()
    {
        std::ifstream ifs(FAILURE_MODE_FILE);
        if (!ifs.is_open()) {
            LOG_DEBUG << "file not found: " << FAILURE_MODE_FILE << ", try dev path...";
            ifs.clear();
            ifs.open(FAILURE_MODE_FILE_DEV);
        }
        if (!ifs.is_open()) {
            LOG_ERROR << "failed to open input file: "
                      << FAILURE_MODE_FILE << " or " << FAILURE_MODE_FILE_DEV;
            return RACK_FAIL;
        }
        Json::CharReaderBuilder builder;
        Json::Value items(Json::arrayValue);
        std::string err;
        if (!Json::parseFromStream(builder, ifs, &items, &err)) {
            LOG_ERROR << "failed to parse json: " << err;
            return RACK_FAIL;
        }

        std::vector<FailureMode> modes;
        modes.reserve(items.size());
        for (const Json::Value& item : items) {
            FailureMode mode = FailureMode::FromJson(item);
            auto it = customizedLogPath_.find(mode.component);
            if (it != customizedLogPath_.end()) {
                mode.dataSource.pathCells.clear();
                for (const PathCell& pathCell : it->second) {
                    mode.dataSource.pathCells.push_back(pathCell);
                }
            }
            modes.push_back(mode);
        }

        std::unordered_map<std::string, std::shared_ptr<LogReader>> dataSourceReaderMap;
        for (const FailureMode& mode : modes) {
            for (const PathCell& pathCell : mode.dataSource.pathCells) {
                std::filesystem::path p = pathCell.path;
                std::vector<PathCell> fileCells;
                if (mode.dataSource.option == DataSourceOption::USER) {
                    if (std::filesystem::is_regular_file(p)) {
                        fileCells.push_back(pathCell);
                    } else if (std::filesystem::is_directory(p)) {
                        for (const auto& entry : std::filesystem::directory_iterator(p)) {
                            if (entry.path().extension() == ".log") {
                                fileCells.emplace_back(pathCell.podId, entry.path().string());
                            }
                        }
                    }
                } else {
                    fileCells.push_back(pathCell);
                }
                for (const PathCell& fileCell : fileCells) {
                    std::string key = fileCell.podId.value_or("null") + " " + fileCell.path;
                    auto it = dataSourceReaderMap.find(key);
                    if (it == dataSourceReaderMap.end()) {
                        auto reader = std::make_shared<LogReader>(mode.dataSource.option, fileCell, query_.startTime, query_.endTime);
                        it = dataSourceReaderMap.emplace(key, std::move(reader)).first;
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

    void LogCollector::CollectUmqMetadata(std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap,
        std::vector<FailureMetadata>& localMetadata)
    {
        for (FailureEvent& event : eventsMap.at("umq")) {
            if (event.attributes.at("alarm_level") != "error") {
                continue;
            }
            FailureMetadata metadata;
            auto it = umqFuncEventTypeMap.find(event.attributes.at("function_name"));
            if (it == umqFuncEventTypeMap.end()) {
                continue;
            }
            metadata.eventType = it->second;
            if (metadata.eventType == EventTypeOption::POST) {
                auto it = umqFuncRoleMap.find(event.attributes.at("function_name"));
                if (it != umqFuncRoleMap.end()) {
                    metadata.role = it->second;
                }
            }
            metadata.podId = event.pathCell.podId;
            metadata.programName = event.attributes.at("program_name");
            metadata.procId = event.attributes.at("proc_id");
            metadata.timestamp = event.timestamp;
            metadata.text = event.text;
            CacheUmqEndpointFields(event);
            auto localEidIt = event.attributes.find("local_eid");
            auto localJettyIdIt = event.attributes.find("local_jetty_id");
            auto remoteEidIt = event.attributes.find("remote_eid");
            auto remoteJettyIdIt = event.attributes.find("remote_jetty_id");
            if (localEidIt != event.attributes.end() && localJettyIdIt != event.attributes.end()) {
                metadata.localEid = localEidIt->second;
                metadata.localJettyId = localJettyIdIt->second;
            }
            if (remoteEidIt != event.attributes.end()) {
                metadata.remoteEid = remoteEidIt->second;
            }
            if (remoteJettyIdIt != event.attributes.end()) {
                metadata.remoteJettyId = remoteJettyIdIt->second;
            }

            if (query_.Match(metadata, podMode_)) {
                localMetadata.push_back(std::move(metadata));
            }
        }
    }

    void LogCollector::CollectCorrelatedLogs(
        std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap,
        std::vector<FailureMetadata>& localMetadata)
    {
        for (FailureMetadata& metadata : localMetadata) {
            for (auto& [component, events] : eventsMap) {
                if (component == "hardware") continue;
                bool previousTimeWindow = (component == "ubsocket");
                int64_t startTime = previousTimeWindow ? metadata.timestamp : metadata.timestamp - TIME_WINDOW_US;
                int64_t endTime = previousTimeWindow ? metadata.timestamp + TIME_WINDOW_US : metadata.timestamp;
                auto beginIt = std::lower_bound(events.begin(), events.end(), startTime,
                    [](const FailureEvent& event, int64_t timestamp) { return event.timestamp < timestamp; });
                auto endIt = std::upper_bound(beginIt, events.end(), endTime,
                    [](int64_t timestamp, const FailureEvent& event) { return timestamp < event.timestamp; });
                for (auto it = beginIt; it != endIt; ++it) {
                    FailureEvent& event = *it;
                    if (podMode_ && event.pathCell.podId && metadata.podId && event.pathCell.podId != metadata.podId)
                        continue;
                    bool resourceMatch = (component == "urmacore" || component == "udmacore" ||
                                          component == "libudma" || component == "ubsocket");
                    if (component == "umq" || component == "liburma") {
                        auto itProgramName = event.attributes.find("program_name");
                        auto itProcId = event.attributes.find("proc_id");
                        if (itProgramName != event.attributes.end() && itProcId != event.attributes.end() &&
                            itProgramName->second == metadata.programName && itProcId->second == metadata.procId) {
                            resourceMatch = true;
                        }
                    }
                    if (component == "umq") {
                        CacheUmqEndpointFields(event);
                        auto localEidIt = event.attributes.find("local_eid");
                        auto localJettyIdIt = event.attributes.find("local_jetty_id");
                        std::optional<std::string> remoteEid;
                        std::optional<std::string> remoteJettyId;
                        auto remoteEidIt = event.attributes.find("remote_eid");
                        auto remoteJettyIdIt = event.attributes.find("remote_jetty_id");
                        if (remoteEidIt != event.attributes.end()) remoteEid = remoteEidIt->second;
                        if (remoteJettyIdIt != event.attributes.end()) remoteJettyId = remoteJettyIdIt->second;
                        if (localEidIt != event.attributes.end() && localJettyIdIt != event.attributes.end() &&
                            localEidIt->second == metadata.localEid &&
                            localJettyIdIt->second == metadata.localJettyId &&
                            remoteEid == metadata.remoteEid && remoteJettyId == metadata.remoteJettyId) {
                            resourceMatch = true;
                        }
                    }
                    if (resourceMatch) metadata.events.push_back(&event);
                }
            }
            std::sort(metadata.events.begin(), metadata.events.end(),
                [](const FailureEvent* a, const FailureEvent* b) { return a->timestamp < b->timestamp; });
        }
    }

    RackResult LogCollector::CorrelateEvents(std::unordered_map<std::string, std::vector<FailureEvent>>& eventsMap)
    {
        auto umqIt = eventsMap.find("umq");
        if (umqIt == eventsMap.end()) {
            LOG_INFO << "no umq events found";
            return RACK_OK;
        }
        for (auto& [_, events] : eventsMap) {
            std::sort(events.begin(), events.end(), [](const FailureEvent& a, const FailureEvent& b) {
                return a.timestamp < b.timestamp;
                });
        }

        std::vector<FailureMetadata> localMetadata;
        localMetadata.reserve(umqIt->second.size());
        CollectUmqMetadata(eventsMap, localMetadata);
        std::sort(localMetadata.begin(), localMetadata.end(), [](const FailureMetadata& a, const FailureMetadata& b) {
            return a.timestamp < b.timestamp;
            });
        CollectCorrelatedLogs(eventsMap, localMetadata);

        {
            std::lock_guard<std::mutex> lk(metadataMutex_);
            metadata_.swap(localMetadata);
        }
        return RACK_OK;
    }

    void LogCollector::Save()
    {
        std::vector<FailureMetadata> snapshot;
        {
            std::lock_guard<std::mutex> lock(metadataMutex_);
            snapshot.swap(metadata_);
        }

        Json::Value j(Json::objectValue);
        j["events"] = Json::Value(Json::arrayValue);
        for (const FailureMetadata& metadata : snapshot) {
            j["events"].append(metadata.ToJson());
        }
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        {
            std::lock_guard<std::mutex> lock(ofsMutex_);
            std::ofstream ofs(FAILURE_EVENT_FILE, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                LOG_ERROR << "failed to open output file: " << FAILURE_EVENT_FILE;
                return;
            }
            writer->write(j, &ofs);
            ofs.flush();
        }
    }

    bool LogCollector::IsValidPodId(const std::string& podId) const
    {
        if (podId.empty()) {
            LOG_ERROR << "empty pod-id";
            return false;
        }
        if (podId.size() > 253) {
            LOG_ERROR << "invalid pod-id: " << podId << ", expected no more than 253 characters but got " << podId.size();
            return false;
        }
        for (size_t i = 0; i < podId.size(); ++i) {
            char ch = podId[i];
            bool isLowerAlnum = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
            if (!isLowerAlnum && !(ch == '-' && i != 0 && i != podId.size() - 1)) {
                LOG_ERROR << "invalid pod-id: " << podId << ", unexpected character: " << ch;
                return false;
            }
        }
        return true;
    }

    bool LogCollector::IsValidPath(const std::string& p) const
    {
        if (p.empty()) {
            LOG_ERROR << "empty path";
            return false;
        }
        std::filesystem::path path(p);
        if (!path.is_absolute()) {
            LOG_ERROR << "invalid path: " << p << ", expected absolute path";
            return false;
        }
        if (!std::filesystem::exists(path)) {
            LOG_ERROR << "path not found: " << p;
            return false;
        }
        return true;
    }

    bool LogCollector::IsValidEid(const std::string& eid) const
    {
        if (eid.empty()) {
            LOG_ERROR << "empty eid";
            return false;
        }
        auto it = std::find_if(eid.begin(), eid.end(), [](unsigned char c) {
            return !std::isdigit(c) && !(c >= 'a' && c <= 'f') && c != ':';
        });
        if (it != eid.end()) {
            LOG_ERROR << "invalid eid: " << eid
                      << ", unexpected char: " << *it
                      << "', ascii: " << static_cast<int>(static_cast<unsigned char>(*it));
            return false;
        }
        std::vector<std::string> parts;
        failure::Split(parts, eid, ':');
        if (parts.empty() ||
            parts.size() != 8 ||
            std::any_of(parts.begin(), parts.end(), [](const std::string& part) { return part.size() != 4; })) {
            LOG_ERROR << "invalid eid: " << eid << ", length must be 128";
            return false;
        }
        return true;
    }

    bool LogCollector::IsValidJettyId(const std::string& jettyId) const
    {
        if (jettyId.empty()) {
            LOG_ERROR << "empty jettyId";
            return false;
        }
        auto it = std::find_if(jettyId.begin(), jettyId.end(), [](unsigned char c) {
            return !std::isdigit(c);
        });
        if (it != jettyId.end()) {
            LOG_ERROR << "invalid jettyId: " << jettyId
                      << ", unexpected char: '" << *it
                      << "', ascii: " << static_cast<int>(static_cast<unsigned char>(*it));
            return false;
        }
        try {
            std::stoi(jettyId);
        } catch (const std::exception& e) {
            LOG_ERROR << "invalid jettyId: " << jettyId << ", " << e.what();
            return false;
        }
        return true;
    }
}
