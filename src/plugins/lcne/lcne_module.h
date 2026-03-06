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

#ifndef LCNE_MODULE_H
#define LCNE_MODULE_H
#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "node_local_collector_module.h"
#include "lcne_node_collector.h"
#include "lcne_topology.h"
#include "http/rack_http_module.h"
#include "witty_json_module.h"
#include "lcne_error.h"
namespace lcne::module
{
    using namespace lcne::collector;
    using namespace rack::module;
    using namespace topology::node;
    using namespace rack::com;
    using namespace witty_json::module;
    class LcneModule : public RackModule
    {
    public:
        LcneModule()
        {
            dependencies.push_back(typeid(JSONModule));
            dependencies.push_back(typeid(NodeLocalCollectorModule));
        }
        ~LcneModule() override = default;
        RackResult Initialize() override;
        void UnInitialize() override;
        RackResult Start() override;
        void Stop() override;

    private:
        std::shared_ptr<lcne::topo::LcneTopology> lcneTopology;
    };
}
#endif