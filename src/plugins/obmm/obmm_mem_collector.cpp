#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include "obmm_mem_collector.h"
namespace obmm::collector {
ObmmResult ObmmMemCollector::GetCurrMemAllDataMap(std::vector<std::unordered_map<std::string, std::string>> &exportMemories,
                                                 std::vector<std::unordered_map<std::string, std::string>> &importMemories)
{
    ObmmResult ret = GetCurrMemExportMemoryDataMap(exportMemories);
    ret = ret || GetCurrMemImportMemoryDataMap(importMemories);
    return ret;
}
ObmmResult ObmmMemCollector::GetCurrMemExportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &exportMemories)
{
    ObmmResult ret = SUCCESS;
    return ret;
}
ObmmResult ObmmMemCollector::GetCurrMemImportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &importMemories)
{
    ObmmResult ret = SUCCESS;
    return ret;
}
}