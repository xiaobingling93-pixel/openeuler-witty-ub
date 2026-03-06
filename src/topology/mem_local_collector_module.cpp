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

#include <tuple>
#include <typeindex>
#include "mem_local_collector_module.h"
#include "rack_error.h"
#include "database_module.h"
#include "ubse_context.h"

namespace topology::mem {
using namespace ubse::context;
using namespace database;
RackResult MemLocalCollectorModule::Initialize()
{
    collector = std::make_shared<MemCollector>();
    collector -> InitDb(UbseContext::GetInstance().GetModule<DatabaseModule>() -> GetDatabase());
    return RACK_OK;
}
void MemLocalCollectorModule::UnInitialize()
{
    return;
}
RackResult MemLocalCollectorModule::Start()
{
    collector -> StartDb();
    return RACK_OK;
}
void MemLocalCollectorModule::Stop()
{
    return;
}
RackResult MemLocalCollectorModule::InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMemories)
{
    RackResult ret = collector -> InsertExportMemoryData(exportMemories);
    return ret;
}
RackResult MemLocalCollectorModule::InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMemories)
{
    RackResult ret = collector -> InsertImportMemoryData(importMemories);
    return ret;
}
}