#define MODULE_NAME "LOG"

#include "log_local_collector_module.h"

#include "logger.h"

namespace failure::log {
    LogLocalCollectorModule::LogLocalCollectorModule()
    {
    }

    RackResult LogLocalCollectorModule::Initialize()
    {
        collector_ = std::make_shared<LogCollector>();
        auto res = collector_->Initialize();
        if (res != RACK_OK) {
            LOG_ERROR << "failed to initialize LogLocalCollectorModule";
            return res;
        }
        LOG_DEBUG << "LogLocalCollectorModule initialize";
        return res;
    }

    void LogLocalCollectorModule::UnInitialize()
    {
        collector_->UnInitialize();
    }

    RackResult LogLocalCollectorModule::Start()
    {
        auto res = collector_->Start();
        if (res != RACK_OK) {
            LOG_ERROR << "failed to start LogLocalCollectorModule";
            return res;
        }
        LOG_DEBUG << "LogLocalCollectorModule started";
        return res;
    }

    void LogLocalCollectorModule::Stop()
    {
        collector_->Stop();
    }

    std::shared_ptr<LogCollector> LogLocalCollectorModule::GetCollector()
    {
        return collector_;
    }
}