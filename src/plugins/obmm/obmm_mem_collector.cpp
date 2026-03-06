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