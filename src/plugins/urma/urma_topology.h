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

#ifndef URMA_TOPOLOGY_H
#define URMA_TOPOLOGY_H

#include <memory>
#include <string>
#include "urma_error.h"
#include "witty_json_module.h"
#include "ubse_context.h"
namespace urma::topo {
constexpr const char *URMA_JSON_PATH = "/var/witty-ub/urma-topology.json";
constexpr mode_t URMA_JSON_PATH_PERM_640 = 0640;
struct SessionKey {
    std::string local_eid;
    std::string local_jetty_id;
    std::string remote_eid;
    std::string remote_jetty_id;

    // 定义排序规则，使map能够存储
    bool operator<(const SessionKey &other) const
    {
        if (local_eid != other.local_eid)
            return local_eid < other.local_eid;
        if (local_jetty_id != other.local_jetty_id)
            return local_jetty_id < other.local_jetty_id;
        if (remote_eid != other.remote_eid)
            return remote_eid < other.remote_eid;
        return remote_jetty_id < other.remote_jetty_id;
    }

    std::string toString() const
    {
        return "local eid=" + local_eid + ", local jetty_id=" + local_jetty_id + ", remote eid=" + remote_eid +
            ", remote jetty_id=" + remote_jetty_id;
    }
};

class URMATopology {
public:
    URMATopology(){
        jsonModule =
            ubse::context::UbseContext::GetInstance().GetModule<witty_json::module::JSONModule>();
    };
    URMAResult ParseUMQLog(std::string file_path, std::map<SessionKey, std::string> &activeSessions);
    URMAResult CreateTopology(ubse::context::TopoToolsArgs &args);
private:
    std::shared_ptr<witty_json::module::JSONModule> jsonModule;
    std::vector<std::string> CollectLogFiles(const std::string& file_path);
    URMAResult ProcessLogFile(const std::string& logFile, std::map<SessionKey, std::string> &activeSessions);
};
} // namespace urma::topo
#endif
