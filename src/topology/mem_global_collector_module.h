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

#ifndef MEM_COLLECTOR_MODULE_H
#define MEM_COLLECTOR_MODULE_H
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "rack_module.h"
#include "rack_error.h"
#include "mem_collector.h"
#include "database_module.h"

namespace topology::mem {
using namespace rack::module;
using namespace database;
class MemGlobalCollectorModule : public RackModule {
public:
    MemGlobalCollectorModule()
    {
        dependencies.push_back(typeid(DatabaseModule));
    }
    ~MemGlobalCollectorModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
    shared_ptr<MemCollector> GetCollector()
    {
        return collector;
    }
    RackResult InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMemories);
    RackResult InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMemories);
private:
    shared_ptr<MemCollector> collector;
};
}
#endif // MEM_GLOBAL_COLLECTOR_MODULE_H