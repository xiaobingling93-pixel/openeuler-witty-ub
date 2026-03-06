#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>
#include "rack_error.h"
#include "obmm_module.h"
#include "ubse_context.h"
#include "mem_local_collector_module.h"
namespace obmm_module {
using namespace ubse::context;
using namespace topology::mem;
RackResult ObmmModule::Initialize()
{
    obmmCollector = std::male_shared<ObmmMemCollector>();
    memLocalCollectorModule = UbseContext::GetInstance().GetModule<MemLocalCollectorModule>();
    return RACK_OK;
}
RackResult ObmmModule::Start()
{
    vector<unordered_map<std::string, std::string>> exportMemories;
    vector<unordered_map<std::string, std::string>> importMemories;
    ObmmResult ret = obmmCollector -> GetCurrMemAllDataMap(exportMemories, importMemories);
    if (ret != SUCCESS) {
        return RACK_OK;
    }
    memLocalCollectorModule -> InsertExportMemoryData(exportMemories);
    memLocalCollectorModule -> InsertImportMemoryData(importMemories);
    return RACK_OK;
}
void ObmmModule::UnInitialize()
{
    return;
}
void ObmmModule::Stop()
{
    return;
}
}