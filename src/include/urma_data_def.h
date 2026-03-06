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

#ifndef URMA_DATA_DEF_H_
#define URMA_DATA_DEF_H_
#include <iostream>
#include <json/json.h>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


namespace topology::urma {
struct Pod {
  std::string podId;
  Pod(std::string podId) : podId(podId) {}
};
struct Jetty {
  std::string localJettyId;
  std::string localDevEid;
  std::string remoteJettyId;
  std::string remoteDevId;
  std::optional<std::string> podId;
  Jetty(std::string localJettyId, std::string localDevEid,
        std::string remoteJettyId, std::string remoteDevId,
        std::optional<std::string> podId = std::nullopt)
      : localJettyId(localJettyId), localDevEid(localDevEid),
        remoteJettyId(remoteJettyId), remoteDevId(remoteDevId), podId(podId) {}
};
struct URMADevice{
    std::string devEid;
    URMADevice(std::string devEid):devEid(devEid) {}
};
inline void to_json(Json::Value& j, const Pod& p) {
    j["pod_id"] = p.podId;
}

inline void to_json(Json::Value& j, const Jetty& y) {
    j["local_jetty_id"] = y.localJettyId;
    j["local_eid"] = y.localDevEid;
    j["remote_jetty_id"] = y.remoteJettyId;
    j["remote_dev_id"] = y.remoteDevId;
    if(y.podId.has_value()){
        j["pod_id"] = y.podId.value();
    }
}
inline void to_json(Json::Value& j, const URMADevice& y) {
    j["device_id"] = y.devEid;
}
} // namespace topology::urma
#endif // URMA_DATA_DEF_H_