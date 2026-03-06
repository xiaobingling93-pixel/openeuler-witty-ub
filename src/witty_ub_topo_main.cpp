#define MODULE_NAME "WITTY-UB-TOPO"
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cctype>
#include <algorithm>
#include <map>
#include "logger.h"
#include "ubse_context.h"
#include "lcne_module.h"
#include "urma_module.h"
using namespace ubse::context;
using namespace topology::node;
using namespace lcne::module;
using namespace urma::module;
using namespace witty_json::module;
UbseContext &g_rackContext = UbseContext::GetInstance();
void RegisterModules()
{
    g_rackContext.RegisterModule<LcneModule>();
    g_rackContext.RegisterModule<NodeLocalCollectorModule>();
    g_rackContext.RegisterModule<URMAModule>();
}

void InitDependencies()
{
    g_rackContext.AddModuleDependencies<JSONModule>();
    g_rackContext.AddModuleDependencies<NodeLocalCollectorModule>();
    g_rackContext.AddModuleDependencies<LcneModule>();
    g_rackContext.AddModuleDependencies<URMAModule>();
}

void CreateModules()
{
    std::vector<std::type_index> sortedModules = g_rackContext.GetSortedModules();
    for (auto type : sortedModules) {
        LOG_INFO << "UbseContext::CreateModules: Creating module " << type.name();
        if (type == typeid(NodeLocalCollectorModule)) {
            g_rackContext.InitModule<NodeLocalCollectorModule>(RackModule::CreateModule<NodeLocalCollectorModule>());
        } else if (type == typeid(LcneModule)) {
            g_rackContext.InitModule<LcneModule>(RackModule::CreateModule<LcneModule>());
        } else if (type == typeid(URMAModule)) {
            g_rackContext.InitModule<URMAModule>(RackModule::CreateModule<URMAModule>());
        } else if (type == typeid(JSONModule)) {
            g_rackContext.InitModule<JSONModule>(RackModule::CreateModule<JSONModule>());
        } else {
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
        LOG_INFO << "Stopping module " << type.name();
        if (moduleMap.find(type) != moduleMap.end()) {
            moduleMap[type]->Stop();
        }
    }
}
int main(int argc, char *argv[])
{
    rack::logger::init(argv[0]);
    LOG_INFO << "Welcome to start Witty-ub-topo tools";
    auto ret = g_rackContext.ParseTopoToolsArgs(argc, argv);
    if (ret == RACK_FAIL) {
        LOG_ERROR << "Parameter error";
        return 1;
    }
    LOG_DEBUG << "Init dependencies";
    InitDependencies();
    LOG_DEBUG << "Register modules";
    RegisterModules();
    LOG_DEBUG << "Create modules";
    CreateModules();
    LOG_DEBUG << "Start modules";
    ret = g_rackContext.Run(argc, argv);
    if (ret == RACK_FAIL) {
        LOG_ERROR << "Run Module error";
        return 1;
    }
    LOG_DEBUG << "UnInitialize and stop Modules";
    UnInitializeAndStopModules();
    rack::logger::shutdown();
    return 0;
}