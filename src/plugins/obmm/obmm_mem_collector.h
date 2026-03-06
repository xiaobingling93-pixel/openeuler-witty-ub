#ifndef OBMM_MEM_COLLECTOR_H
#define OBMM_MEM_COLLECTOR_H

#include <vector>
#include <unordered_map>
#include "mem_data_def.h"

namespace obmm::collector {
using ObmmResult = uint32_t;
constexpr ObmmResult SUCCESS = 0;
constexpr ObmmResult FAIL = 1;
class ObmmMemCollector {
public:
    ObmmResult GetCurrMemAllDataMap(std::vector<std::unordered_map<std::string, std::string>> &exportMemories,
                                    std::vector<std::unrodered_map<std::string, std::string>> &importMemories);
    ObmmResult GetCurrMemExportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &exportMemories);
    ObmmResult GetCurrMemImportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &importMemories);
};
}

#endif //OBMM_MEM_COLLECTOR_H