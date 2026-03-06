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
                                    std::vector<std::unordered_map<std::string, std::string>> &importMemories);
    ObmmResult GetCurrMemExportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &exportMemories);
    ObmmResult GetCurrMemImportMemoryDataMap(std::vector<std::unordered_map<std::string, std::string>> &importMemories);
};
}

#endif //OBMM_MEM_COLLECTOR_H