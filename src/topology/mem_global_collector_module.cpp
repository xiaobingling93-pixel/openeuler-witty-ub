#include <tuple>
#include <iostream>
#include "mem_global_collector_module.h"
#include "database_module.h"
#include "database.h"
#include "rack_error.h"
#include "ubse_context.h"

namespace topology::mem {
using namespace ubse::context;
using namespace database;

RackResult MemGlobalCollectorModule::Initialize()
{
    collector = std::make_shared<MemCollector>();
    OP_RET ret = collector -> InitDb(UbseContext::GetInstance().GetModule<DatabaseModule>() -> GetDatabase());
    std::cout << "MemGlobalCollectorModule Initialized " << ret <<std::endl;
    return RACK_OK;
}
void MemGlobalCollectorModule::UnInitialize()
{
    return;
}
RackResult MemGlobalCollectorModule::Start()
{
    collector -> StartDb();
    std::cout << "MemGlobalCollectorModule Started " << std::endl;
    return RACK_OK;
}
void MemGlobalCollectorModule::Stop()
{
    return;
}
RackResult MemGlobalCollectorModule::InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMemories)
{
    OP_RET ret = collector -> InsertExportMemoryData(exportMemories);
    if (ret != OP_RET::SUCCESS) {
        return RACK_FAIL;
    }
    return RACK_OK;
}
RackResult MemGlobalCollectorModule::InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMemories)
{
    OP_RET ret = collector -> InsertImportMemoryData(importMemories);
    if (ret != OP_RET::SUCCESS) {
        return RACK_FAIL;
    }
    return RACK_OK;
}
}