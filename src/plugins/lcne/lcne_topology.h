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

#ifndef LCNE_TOPOLOGY_H
#define LCNE_TOPOLOGY_H

#include "http/rack_http_types.h"
#include "lcne_error.h"
#include "lcne_node_collector.h"
#include "node_local_collector_module.h"
#include "ubse_context.h"
#include "witty_json_module.h"
#include <memory>

namespace lcne::topo {
class LcneTopology {
public:
  LcneTopology() {
    lcneCollector = std::make_shared<lcne::collector::LcneNodeCollector>();
    nodeLocalCollectorModule =
        ubse::context::UbseContext::GetInstance().GetModule<topology::node::NodeLocalCollectorModule>();
    jsonModule = ubse::context::UbseContext::GetInstance().GetModule<witty_json::module::JSONModule>();
  }
  LcneResult SubTopolgyChanges();
  LcneResult CreateTopolgy();
  LcneResult GetHisTopolgy(
      std::vector<std::shared_ptr<topology::node::Node>> &nodes,
      std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
      std::vector<std::shared_ptr<topology::node::Port>> &ports);
  LcneResult GetCurTopolgy(
      std::vector<std::shared_ptr<topology::node::Node>> &nodes,
      std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
      std::vector<std::shared_ptr<topology::node::Port>> &ports);
  LcneResult RegLinkNotifyHttpHandler();
  rack::com::RackComResult<rack::com::RackHttpResponse>
  LinkNotifyHandlerFunc(const rack::com::RackComContext &ctx,
                        const rack::com::RackHttpRequest &req);
  rack::com::RackComResult<rack::com::RackHttpResponse>
  GetLcneTopologyInitHandlerFunc(const rack::com::RackComContext &ctx,
                                 const rack::com::RackHttpRequest &req);

  rack::com::RackComResult<rack::com::RackHttpResponse>
  GetLcneTopologyHandlerFunc(const rack::com::RackComContext &ctx,
                             const rack::com::RackHttpRequest &req);

private:
  std::shared_ptr<lcne::collector::LcneNodeCollector> lcneCollector;
  std::shared_ptr<topology::node::NodeLocalCollectorModule>
      nodeLocalCollectorModule;
  std::shared_ptr<witty_json::module::JSONModule> jsonModule;
};

} // namespace lcne::topo
#endif