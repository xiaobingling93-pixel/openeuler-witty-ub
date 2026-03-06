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

#define MODULE_NAME "Witty-UB"
#include <string>
#include <iostream>
#include "ubse_context.h"
#include "rack_module.h"
#include "rack_error.h"
#include "node_global_collector_module.h"
#include "node_local_collector_module.h"
#include "mem_local_collector_module.h"
#include "mem_global_collector_module.h"
#include "log_local_collector_module.h"
#include "database_module.h"
#include "lcne_module.h"
#include "obmm_module.h"
#include "logger.h"
#include "http/rack_http_module.h"

using namespace ubse::context;
using namespace topology::node;
using namespace topology::mem;
using namespace lcne::module;
using namespace obmm::module;
using namespace failure::log;
UbseContext& g_rackContext = UbseContext::GetInstance();
// Give the module names to be loaded as command line arguments

void RegisterModules(string role)
{
    if (role == "analyzer") {
        g_rackContext.RegisterModule<NodeGlobalCollectorModule>();
        //g_rackContext.RegisterModule<MemGlobalCollectorModule>();
    } else if (role == "collector") {
        g_rackContext.RegisterModule<LcneModule>();
        //g_rackContext.RegisterModule<ObmmModule>();
        g_rackContext.RegisterModule<NodeLocalCollectorModule>();
        //g_rackContext.RegisterModule<MemLocalCollectorModule>();
        g_rackContext.RegisterModule<LogLocalCollectorModule>();
    } else {
        std::cerr << "RegisterModules-Error: role " << role << " not supported" << std::endl;
    }
}

void InitDependencies()
{
    g_rackContext.AddModuleDependencies<DatabaseModule>();
    g_rackContext.AddModuleDependencies<rack::com::HttpModule>();
    //g_rackContext.AddModuleDependencies<NodeGlobalCollectorModule>();
    g_rackContext.AddModuleDependencies<NodeLocalCollectorModule>();
    //g_rackContext.AddModuleDependencies<MemLocalCollectorModule>();
    //g_rackContext.AddModuleDependencies<MemGlobalCollectorModule>();
    g_rackContext.AddModuleDependencies<LcneModule>();
    g_rackContext.AddModuleDependencies<LogLocalCollectorModule>();
    //g_rackContext.AddModuleDependencies<ObmmModule>();
}

void CreateModules()
{
    std::vector<std::type_index> sortedModules = g_rackContext.GetSortedModules();
    for (auto type: sortedModules) {
        LOG_INFO << "UbseContext::CreateModules: Creating module " << type.name();
        if (type == typeid(DatabaseModule)) {
            g_rackContext.InitModule<DatabaseModule>(RackModule::CreateModule<DatabaseModule>());
        } else if (type == typeid(NodeLocalCollectorModule)) {
            g_rackContext.InitModule<NodeLocalCollectorModule>(RackModule::CreateModule<NodeLocalCollectorModule>());
        } else if (type == typeid(NodeGlobalCollectorModule)) {
            g_rackContext.InitModule<NodeGlobalCollectorModule>(RackModule::CreateModule<NodeGlobalCollectorModule>());
        } else if (type == typeid(LcneModule)) {
            g_rackContext.InitModule<LcneModule>(RackModule::CreateModule<LcneModule>());
        } else if (type == typeid(ObmmModule)) {
            g_rackContext.InitModule<ObmmModule>(RackModule::CreateModule<ObmmModule>());
        } else if (type == typeid(MemLocalCollectorModule)) {
            g_rackContext.InitModule<MemLocalCollectorModule>(RackModule::CreateModule<MemLocalCollectorModule>());
        } else if (type == typeid(MemGlobalCollectorModule)) {
            g_rackContext.InitModule<MemGlobalCollectorModule>(RackModule::CreateModule<MemGlobalCollectorModule>());
        } else if (type == typeid(LogLocalCollectorModule)) {
            g_rackContext.InitModule<LogLocalCollectorModule>(RackModule::CreateModule<LogLocalCollectorModule>());
        } else if (type == typeid(rack::com::HttpModule)) {
            g_rackContext.InitModule<rack::com::HttpModule>(RackModule::CreateModule<rack::com::HttpModule>());
        } else {
            std::cerr << "CreateModule-Error: module " << type.name() << " not defined" << std::endl;
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
        LOG_INFO << "Stopping module " << type.name();
        if (moduleMap.find(type) != moduleMap.end()) {
            moduleMap[type]->Stop();
        }
    }
}
int main(int argc, char *argv[])
{
    //Init log first
    rack::logger::init(argv[0]);
    LOG_DEBUG << "Start InitDependencies";
    // For all possible modules, add the dependencies as shown below
    InitDependencies();
    LOG_DEBUG << "Start analyze parameters";
    g_rackContext.ParseArgs(argc, argv);
    string role = g_rackContext.GetRole();
    LOG_INFO << "Start Witty-UB, role is " << role;
    RegisterModules(role);
    CreateModules();
    RackResult ret = g_rackContext.Run(argc, argv);
    UnInitializeAndStopModules();
    rack::logger::shutdown();
    return 0;
}
