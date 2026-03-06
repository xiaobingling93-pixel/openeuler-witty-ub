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

#ifndef LCNE_NODE_COLLECTOR_H
#define LCNE_NODE_COLLECTOR_H

#include <vector>
#include <memory>
#include <unordered_map>
#include "node_data_def.h"
#include "lcne_error.h"

namespace lcne::collector
{
    class LcneNodeCollector
    {
    public:
        LcneResult GetCurrNodeAllDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes,
                                         std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
                                         std::vector<std::shared_ptr<topology::node::Port>> &ports);
        LcneResult GetCurrNodeDeviceDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes);
        LcneResult GetCurrNodeUbCDataMap(std::vector<std::shared_ptr<topology::node::UbController>> &ubcs);
        LcneResult GetCurrNodePortDataMap(std::vector<std::shared_ptr<topology::node::Port>> &ports);
    };
}
#endif // LCNE_NODE_COLLECTOR_H