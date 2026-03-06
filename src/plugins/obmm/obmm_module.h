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

#ifndef OBMM_MODULE_H
#define OBMM_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "mem_local_collector_module.h"
#include "obmm_mem_collector.h"
namespace obmm::module {
using namespace obmm::collector;
using namespace rack::module;
using namespace topology::mem;
class ObmmModule : public RackModule {
public:
    ObmmModule()
    {
        dependencies.push_back(typeid(MemLocalCollectorModule));
    }
    ~ObmmModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
private:
    std::shared_ptr<ObmmMemCollector> obmmCollector;
    std::shared_ptr<MemLocalCollectorModule> memLocalCollectorModule;
};
}
#endif // OBMM_MODULE_H