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

#define MODULE_NAME "LCNE"
#include "lcne_topology.h"
#include "http/rack_http_server_handler.h"
#include "http/rack_http_types.h"
#include "lcne_common.h"
#include "logger.h"
#include "node_local_collector_module.h"
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

namespace lcne::topo{
  template <typename T>
  std::vector<T>
  convertPtrVectorToValueVector(const std::vector<std::shared_ptr<T>> &ptrVec)
  {
    std::vector<T> result;
    result.reserve(ptrVec.size());
    for (const auto &ptr : ptrVec)
    {
      if (ptr)
      {
        result.push_back(*ptr);
      }
    }
    return result;
  }
  LcneResult LcneTopology::CreateTopolgy()
  {
    std::vector<std::shared_ptr<topology::node::Node>> nodes;
    std::vector<std::shared_ptr<topology::node::UbController>> ubcs;
    std::vector<std::shared_ptr<topology::node::Port>> ports;
    LcneResult ret = lcneCollector->GetCurrNodeAllDataMap(nodes, ubcs, ports);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::CreateTopolgy-Error: failed to get curr node "
                   "all data map";
      return ret;
    }
    std::vector<topology::node::UbController> ubcs_vec =
        convertPtrVectorToValueVector(ubcs);
    std::vector<topology::node::Port> ports_vec =
        convertPtrVectorToValueVector(ports);
    
    // Check if network mode is clos, if so, clear remote fields in ports
    auto topoArgs = ubse::context::UbseContext::GetInstance().GetTopoToolsArgs();
    if (topoArgs.network_mode == "clos") {
      for (auto &port : ports_vec) {
        port.remotePortId.reset();
        port.remoteSlotId.reset();
        port.remoteUbpuId.reset();
        port.remoteIouId.reset();
      }
    }
    
    auto ubcs_pair = jsonModule->GetJsonPair("iodie", ubcs_vec);
    auto port_pair = jsonModule->GetJsonPair("port", ports_vec);
    auto json_ret = jsonModule->WriteVectorsToFile(lcne::common::JSON_OUTPUT_FILE,
                                                   ubcs_pair, port_pair);
    if (json_ret == RACK_FAIL)
    {
        LOG_ERROR << "LcneTopology::CreateTopolgy-Error: failed to write json file";
        return LCNE_FAIL;
    }
    if (::chmod(lcne::common::JSON_OUTPUT_FILE, lcne::common::JSON_OUTPUT_FILE_PERM_640) != 0) {
        LOG_ERROR << "LcneTopology::CreateTopolgy-Error: failed to set file mode 0640 for output file "
                  << lcne::common::JSON_OUTPUT_FILE;
        return LCNE_FAIL;
    }
#ifdef ENABLE_DAEMON_FEATURE
    ret = nodeLocalCollectorModule->InsertDeviceData(nodes);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::CreateTopolgy-Error: failed to update node topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->InsertUbCData(ubcs);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::CreateTopolgy-Error: failed to update ubc topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->InsertPortData(ports);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::CreateTopolgy-Error: failed to update port topology";
      return ret;
    }
#endif
    return LCNE_SUCCESS;
  }
  LcneResult LcneTopology::GetHisTopolgy(
      std::vector<std::shared_ptr<topology::node::Node>> &nodes,
      std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
      std::vector<std::shared_ptr<topology::node::Port>> &ports)
  {
    LcneResult ret = nodeLocalCollectorModule->QueryHisDeviceData(nodes);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::GetHisTopolgy-Error: failed to get his node topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->QueryHisUbCData(ubcs);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::GetHisTopolgy-Error: failed to get his ubc topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->QueryHisPortData(ports);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR
          << "LcneTopology::GetHisTopolgy-Error: failed to get his port topology";
      return ret;
    }
    return LCNE_SUCCESS;
  }
  LcneResult LcneTopology::GetCurTopolgy(
      std::vector<std::shared_ptr<topology::node::Node>> &nodes,
      std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
      std::vector<std::shared_ptr<topology::node::Port>> &ports)
  {
    LcneResult ret = nodeLocalCollectorModule->QueryCurDeviceData(nodes);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::GetCurTopolgy-Error: failed to get curr node "
                   "device topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->QueryCurUbCData(ubcs);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::GetCurTopolgy-Error: failed to get curr node "
                   "ubc topology";
      return ret;
    }
    ret = nodeLocalCollectorModule->QueryCurPortData(ports);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::GetCurTopolgy-Error: failed to get curr node "
                   "port topology";
      return ret;
    }
    return LCNE_SUCCESS;
  }

  LcneResult LcneTopology::SubTopolgyChanges()
  {
    return lcne::common::postLinkInfoNotify();
  }

  LcneResult LcneTopology::RegLinkNotifyHttpHandler()
  {
    rack::com::RackHttpHandler linkNotifyHandlerFunc =
        std::bind(&LcneTopology::LinkNotifyHandlerFunc, this,
                  std::placeholders::_1, std::placeholders::_2);
    rack::com::RackHttpServerHandler::GetInstance().Register(
        rack::com::RackHttpMethod::POST, lcne::common::LCNE_NOTIFY_TOPO_PATH,
        linkNotifyHandlerFunc);
    return LCNE_SUCCESS;
  }
  rack::com::RackComResult<rack::com::RackHttpResponse>
  LcneTopology::LinkNotifyHandlerFunc(const rack::com::RackComContext &ctx,
                                      const rack::com::RackHttpRequest &req)
  {
    LOG_INFO << "LcneTopology::LinkNotifyHandlerFunc-Info: received link notify";
    if (CreateTopolgy() == LCNE_FAIL)
    {
      LOG_ERROR << "LcneTopology::LinkNotifyHandlerFunc-Error: failed to create "
                   "topology";
    }
    // todo post to master server
    rack::com::RackHttpResponse resp;
    resp.status = 200;
    return rack::com::RackComResult<rack::com::RackHttpResponse>::Ok(
        std::move(resp));
  }
  rack::com::RackComResult<rack::com::RackHttpResponse>
  LcneTopology::GetLcneTopologyInitHandlerFunc(const rack::com::RackComContext &ctx,
                                 const rack::com::RackHttpRequest &req)
  {
    LOG_INFO << "LcneTopology::GetLcneTopologyInitHandlerFunc-Info: received "
                "get lcne topology init request";
    rack::com::RackHttpResponse resp;
    LcneResult ret = lcne::common::SaveIpToConfigFile(
        req.remote_addr + ":" + std::to_string(req.remote_port));
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::GetLcneTopologyInitHandlerFunc-Error: failed "
                   "to create "
                   "topology";
      resp.status = 404;
      return rack::com::RackComResult<rack::com::RackHttpResponse>::Ok(
          std::move(resp));
    }
    std::vector<std::shared_ptr<topology::node::Node>> cur_nodes;
    std::vector<std::shared_ptr<topology::node::UbController>> cur_ubcs;
    std::vector<std::shared_ptr<topology::node::Port>> cur_ports;
    std::vector<std::shared_ptr<topology::node::Node>> his_nodes;
    std::vector<std::shared_ptr<topology::node::UbController>> his_ubcs;
    std::vector<std::shared_ptr<topology::node::Port>> his_ports;
    ret = GetCurTopolgy(cur_nodes, cur_ubcs, cur_ports);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "LcneTopology::GetLcneTopologyInitHandlerFunc-Error: failed "
                   "to get his node "
                   "topology";
      resp.status = 404;
      return rack::com::RackComResult<rack::com::RackHttpResponse>::Ok(
          std::move(resp));
    }
    resp.status = 200;
    return rack::com::RackComResult<rack::com::RackHttpResponse>::Ok(
        std::move(resp));
  }

  rack::com::RackComResult<rack::com::RackHttpResponse>
  LcneTopology::GetLcneTopologyHandlerFunc(const rack::com::RackComContext &ctx,
                             const rack::com::RackHttpRequest &req)
  {
    rack::com::RackHttpResponse resp;
    resp.status = 200;
    return rack::com::RackComResult<rack::com::RackHttpResponse>::Ok(
        std::move(resp));
  }

} // namespace lcne::topo

