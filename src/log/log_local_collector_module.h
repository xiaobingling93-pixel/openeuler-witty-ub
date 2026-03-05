#pragma once

#include "rack_error.h"
#include "rack_module.h"
#include "log_collector.h"

namespace failure::log {
    using namespace rack::module;

    class LogLocalCollectorModule final : public RackModule {
    public:
        LogLocalCollectorModule();
        ~LogLocalCollectorModule() override = default;

        RackResult Initialize() override;
        void UnInitialize() override;
        RackResult Start() override;
        void Stop() override;

        std::shared_ptr<LogCollector> GetCollector();

    private:
        std::shared_ptr<LogCollector> collector_;
    };
}