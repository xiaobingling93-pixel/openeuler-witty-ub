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
        LOG_DEBUG << "LogLocalCollectorModule initialized";
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