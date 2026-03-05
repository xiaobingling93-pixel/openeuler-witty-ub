#ifndef URMA_DATA_DEF_H_
#define URMA_DATA_DEF_H
#include <iostream>
#include <nlohmann/json.hpp>
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
inline void to_json(nlohmann::json& j, const Pod& p) {
    j = nlohmann::json{
        {"pod_id", p.podId}
    };
}

inline void to_json(nlohmann::json& j, const Jetty& y) {
    j = nlohmann::json{
        {"local_jetty_id", y.localJettyId},
        {"local_eid", y.localDevEid},
        {"remote_jetty_id", y.remoteJettyId},
        {"remote_dev_id", y.remoteDevId}
    };
    if(y.podId.has_value()){
        j["pod_id"] = y.podId.value();
    }
}
inline void to_json(nlohmann::json& j, const URMADevice& y) {
    j = nlohmann::json{
        {"device_id", y.devEid}
    };
}
} // namespace topology::urma
#endif // URMA_DATA_DEF_H_