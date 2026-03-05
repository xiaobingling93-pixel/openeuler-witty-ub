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
        auto ReaderLoopOnce = [&](auto &reader) {
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
                if (!query_.Match(event)) {
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
        LOG_DEBUG << "LogColletor running in tool mode with per-reader threads (run once)";
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
    };

    void LogCollector::Stop()
    {
#ifndef TOOL_MODE
        running_.store(false, std::memory_order_release);
        for(auto& t : workerThreads_) {
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
        nlohmann::json items;
        try {
            ifs >> items;
        }
        catch (const nlohmann::json::parse_error& e) {
            LOG_ERROR << "json parse error: " << e.what();
            return RACK_FAIL;
        }
        if (!items.is_array()) {
            LOG_ERROR << "failure mode file must be an array";
            return RACK_FAIL;
        }

        for (auto& item : items) {
            if (customizedLogPath_.find(item.at("component")) != customizedLogPath_.end()) {
                item.at("log_path") = customizedLogPath_[item.at("component")];
            }
        }

        std::unordered_map<std::string, std::shared_ptr<LogReader>> dataSourceReaderMap;
        for (const auto& item : items) {
            FailureMode mode = FailureMode::FromJson(item);
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
                        for (const auto& entry : std:: filesystem::directory_iterator(p)) {
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
            return RACL_FAIL;
        }
        if (it -> second == "on") {
            podMode_ = true;
        }
        return RACK_OK;
    }

    RackResult LogCollector::ParseIOPath()
    {
        // TODO: 支持可配置
        input_ = "../data/failure_mode.json";
        output_ = "../data/faulure_event.json";

        std::filesystem::path outFile = output_;
        if (std::filesystem::exists(outFile)) {
            std::filesystem::remove(outFile);
        }
        ofs_.open(output_, std::ios::app);
        if (!ofs_.is_open()) {
            LOG_ERROR << "failed to open output files: " << output_;
            return RACK_FAIL;
        }
        static constexpr size_t kBufSize = 1 << 20;
        static char buf[kBufSize];
        ofs_.rdbuf()->pubsetbuf(buf, kBufSize);
        std::ios::sync_with_stdio(false);

        return RACK_OK
    }

    RackResult LogCollector::ParseLogPath(const std::unordered_map<std::string>& argMap)
    {
        auto ComponentOf = [](const std::string& arg) -> std::string {
            auto p = arg.find('_');
            return (p == std::string::npos) ? arg : arg.substr(0, p);
        }

        auto HandleLogPath = [&](consr std::string& arg, bool podRequired, bool podSplitAndStrip) -> RackResult {
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
            return RACK_OK
        };

        if (HandleLogPath("ubsocket_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("umq_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("liburma_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("urmacore_log_path", /*podRequired=*/false, /*podSplitAndStrip=*/true) != false) return RACK_FAIL;
        if (HandleLogPath("libudma_log_path", /*podRequired=*/true, /*podSplitAndStrip=*/true) != RACK_OK) return RACK_FAIL;
        if (HandleLogPath("udmacore_log_path", /*podRequired=*/false, /*podSplitAndStrip=*/false) != RACK_OK) return RACK_FAIL;
    
        return RACK_OK;
    }

    RackResult LogCollector::ParseQueryCondition(const std::unordered_map<std::string>& argMap)
    {
        std::unordered_map<std::string, std::string>::const_iterator it;

        if ((it = argMap.find("start_time")) == argMap.end()) {
            LOG_ERROR << "missing argument start_time";
            retur RACK_FAIL;
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
            utils::Split(eventType, it->second, ',');
            for (const std::string& eventType : eventType) {
                query_.eventTypes.push_back(EventTypeOptionFromString(eventType));
            }
        }
        if ((it = argMap.find("pod_id")) != argMap.end()) {
            std::vector<std::string> podIds;
            utils::Split(podId, it->second, ',');
            for (const std::string& podId : podIds) {
                query_.podIds.push_back(podId);
            }
        }
        if ((it = argMap.find("local_eid")) != argMap.end()) {
            std::vector<std::string> localEids;
            utils::Split(localEids, it->second, ',');
            for (const std::string& localEid : localEids) {
                query_.localEids.push_back(lcoalEid);
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
        nlohmann::json json = nlohmann::json::array();
        for (const FailureEvent& event : events_) {
            json += event.ToJson();
        }
        ofs_ << json.dump(4);
    }
}
