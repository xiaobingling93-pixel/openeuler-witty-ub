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

#define MODULE_NAME "Copilot-UB-Log"

#include <iostream>

#include "log_local_collector_module.h"
#include "ubse_context.h"
#include "logger.h"

using namespace ubse::context;
using namespace failure::log;

UbseContext& g_rackContext = UbseContext::GetInstance();

void RegisterModules()
{
    g_rackContext.RegisterModule<LogLocalCollectorModule>();
}

void InitDependencies()
{
    g_rackContext.AddModuleDependencies<LogLocalCollectorModule>();
}

void CreateModules()
{
    std::vector<std::type_index> sortedModules = g_rackContext.GetSortedModules();
    for (auto type : sortedModules) {
        LOG_DEBUG << "UbseContext::CreateModules: Creating module " << type.name();
        if (type == typeid(LogLocalCollectorModule)) {
            g_rackContext.InitModule<LogLocalCollectorModule>(RackModule::CreateModule<LogLocalCollectorModule>());
        }
        else {
            LOG_ERROR << "CreateModule-Error: module " << type.name() << " not defined";
        }
    }
}

void UnInitializeAndStopModules()
{
    std::unordered_map<std::type_index, std::shared_ptr<RackModule>> moduleMap = g_rackContext.GetModuleMap();
    std::vector<std::type_index> sortedModules = g_rackContext.GetSortedModules();
    for (int i = sortedModules.size() - 1; i >= 0; i--) {
        auto type = sortedModules[i];
        if (moduleMap.find(type) != moduleMap.end()) {
            moduleMap[type]->UnInitialize();
        }
    }
    for (int i = sortedModules.size() - 1; i >= 0; i--) {
        auto type = sortedModules[i];
        LOG_DEBUG << "Stopping module" << type.name();
        if (moduleMap.find(type) != moduleMap.end()) {
            moduleMap[type]->Stop();
        }
    }
}

int main(int argc, char* argv[])
{
    rack::logger::init(argv[0]);
    LOG_DEBUG << "Start InitDependencies";
    InitDependencies();
    LOG_DEBUG << "Start analyze parameters";
    g_rackContext.ParseArgs(argc, argv);
    RegisterModules();
    CreateModules();
    RackResult ret = g_rackContext.Run(argc, argv);
    UnInitializeAndStopModules();
    rack::logger::shutdown();
    return 0;
}