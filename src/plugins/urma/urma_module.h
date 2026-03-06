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

#ifndef URMA_MODULE_H
#define URMA_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "urma_topology.h"
#include "witty_json.h"

namespace urma::module {
using namespace rack::module;
class URMAModule : public RackModule{
public:
    URMAModule()
    {
        dependencies.push_back(typeid(witty_json::module::JSONModule));
    }
    ~URMAModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
private:
    std::shared_ptr<urma::topo::URMATopology> urmaTopology;
};
}

#endif //URMA_MODULE_H