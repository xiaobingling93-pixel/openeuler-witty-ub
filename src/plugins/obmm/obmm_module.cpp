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
#include <iostream>
#include <string>
#include "rack_error.h"
#include "obmm_module.h"
#include "ubse_context.h"
#include "mem_local_collector_module.h"
namespace obmm::module {
using namespace ubse::context;
using namespace topology::mem;
RackResult ObmmModule::Initialize()
{
    obmmCollector = std::make_shared<ObmmMemCollector>();
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