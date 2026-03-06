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

#ifndef NODE_DATA_DEF_H
#define NODE_DATA_DEF_H
#include <iostream>
#include <json/json.h>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace topology::node {
using namespace std;
#define GUID_LENGTH = 16
#define EID_LENGTH = 20
enum class ChipType {
  CPU = 0,
  CPULINK = 1,
  NPU = 2,
  UNKNOWN = 3,
};
enum class DieState { NORMAL = 0, ABNORMAL = 1, UNKNOWN = 2 };
enum class UbCState {
  INITIAL = 0,
  ONLINE = 1,
  OFFLINE = 2,
  RESETTING = 3,
  ABNORMAL = 4,
  UNKNOWN = 5
};
enum class PortState { UP = 0, DOWN = 1, UNKNOWN = 2 };

extern unordered_map<string, ChipType> strToChipTypeMap;
extern unordered_map<ChipType, string> chipTypeToStrMap;
extern unordered_map<string, DieState> strToDieStateMap;
extern unordered_map<DieState, string> dieStateToStrMap;
extern unordered_map<string, UbCState> strToUbCStateMap;
extern unordered_map<UbCState, string> ubcStateToStrMap;
extern unordered_map<string, PortState> strToPortStateMap;
extern unordered_map<PortState, string> portStateToStrMap;
vector<string> GetHostIps(string str);
vector<uint32_t> GetPortIds(string str);
std::string MergeStr(std::vector<std::string> strVec);
ChipType Str2ChipType(std::string chipType);
string ChipType2Str(ChipType chipType);
DieState Str2DieState(std::string dieState);
string DieState2Str(DieState dieState);
UbCState Str2UbCState(std::string ubCState);
string UbcState2Str(UbCState ubCState);
PortState Str2PortState(std::string portState);
string PortState2Str(PortState portState);
template <typename T> string Vector2Str(vector<T> const &values) {
  std::ostringstream oss;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0)
      oss << ",";
    oss << values[i];
  }
  return oss.str();
}
class ObjFormatter {
public:
  virtual unordered_map<string, string> ObjToDataMap() = 0;
  virtual ~ObjFormatter() = default;
};

struct Node : public ObjFormatter {
  uint32_t deviceId;
  uint32_t slotId;
  string hostname;
  vector<string> ipAddrs;
  uint32_t chipNum;
  uint32_t dieNum;
  ChipType chipType;

  Node() = default;

  Node(uint32_t deviceId, uint32_t slotId, string hostname,
       vector<string> ipAddrs, uint32_t chipNum, uint32_t dieNum,
       ChipType chipType)
      : deviceId(deviceId), slotId(slotId), hostname(hostname),
        ipAddrs(ipAddrs), chipNum(chipNum), dieNum(dieNum), chipType(chipType) {
  }

  unordered_map<string, string> ObjToDataMap() override {
    unordered_map<string, string> data_map;
    data_map["deviceId"] = to_string(deviceId);
    data_map["slotId"] = to_string(slotId);
    data_map["hostname"] = hostname;
    data_map["ipAddrs"] = Vector2Str(ipAddrs);
    data_map["chipNum"] = to_string(chipNum);
    data_map["dieNum"] = to_string(dieNum);
    data_map["chipType"] = ChipType2Str(chipType);
    return data_map;
  };
};
struct UbController : public ObjFormatter {
  string dieGuid;
  string ubcEid;
  uint32_t deviceId;
  uint32_t slotId;
  uint32_t chipId;
  uint32_t dieId;
  string primaryCna;
  vector<uint32_t> portIds;
  DieState dieState;
  UbCState ubcState;

  UbController() = default;
  UbController(string dieGuid, string ubcEid, uint32_t deviceId,
               uint32_t slotId, uint32_t chipId, uint32_t dieId,
               string primaryCna, vector<uint32_t> portIds, DieState dieState,
               UbCState ubcState)
      : dieGuid(dieGuid), ubcEid(ubcEid), deviceId(deviceId), slotId(slotId),
        chipId(chipId), dieId(dieId), primaryCna(primaryCna),
        portIds(std::move(portIds)), dieState(dieState), ubcState(ubcState) {}

  unordered_map<string, string> ObjToDataMap() override {
    unordered_map<string, string> data_map;
    data_map["dieGuid"] = dieGuid;
    data_map["ubcEid"] = ubcEid;
    data_map["deviceId"] = to_string(deviceId);
    data_map["slotId"] = to_string(slotId);
    data_map["chipId"] = to_string(chipId);
    data_map["dieId"] = to_string(dieId);
    data_map["primaryCna"] = primaryCna;
    data_map["portIds"] = Vector2Str(portIds);
    data_map["dieState"] = DieState2Str(dieState);
    data_map["ubcState"] = UbcState2Str(ubcState);
    return data_map;
  }
};
  inline void to_json(Json::Value &j, const UbController &u) {
    j["primary_cna"] = u.primaryCna;
  }
  struct Port : public ObjFormatter {
    uint32_t portId;
    string portCna;
    string primaryCna;
    uint32_t deviceId;
    PortState portState;
    // remote port maybe empty
    std::optional<uint32_t> remotePortId;
    std::optional<uint32_t> remoteDeviceId;
    std::optional<uint32_t> remoteSlotId;
    std::optional<uint32_t> remoteUbpuId;
    std::optional<uint32_t> remoteIouId;
    Port() = default;
    Port(uint32_t portId, string portCna, string primaryCna, uint32_t deviceId,
         PortState portState, std::optional<uint32_t> remotePortId,
         std::optional<uint32_t> remoteDeviceId,
         std::optional<uint32_t> remoteSlotId,
         std::optional<uint32_t> remoteUbpuId,
         std::optional<uint32_t> remoteIouId)
        : portId(portId), portCna(portCna), primaryCna(primaryCna),
          deviceId(deviceId), portState(portState), remotePortId(remotePortId),
          remoteDeviceId(remoteDeviceId), remoteSlotId(remoteSlotId),
          remoteUbpuId(remoteUbpuId), remoteIouId(remoteIouId) {}
    unordered_map<string, string> ObjToDataMap() override {
      unordered_map<string, string> data_map;
      data_map["portId"] = to_string(portId);
      data_map["portCna"] = portCna;
      data_map["primaryCna"] = primaryCna;
      data_map["deviceId"] = to_string(deviceId);
      data_map["portState"] = PortState2Str(portState);
      data_map["remotePortId"] =
          remotePortId ? to_string(remotePortId.value()) : "-";
      data_map["remoteDeviceId"] =
          remoteDeviceId ? to_string(remoteDeviceId.value()) : "-";
      data_map["remoteSlotId"] =
          remoteSlotId ? to_string(remoteSlotId.value()) : "-";
      data_map["remoteUbpuId"] =
          remoteUbpuId ? to_string(remoteUbpuId.value()) : "-";
      data_map["remoteIiId"] =
          remoteIouId ? to_string(remoteIouId.value()) : "-";
      return data_map;
    }
  };
  inline void to_json(Json::Value &j, const Port &p) {
    j["port_id"] = static_cast<Json::UInt>(p.portId);
    j["primary_cna"] = p.primaryCna;
    if(p.remotePortId.has_value())
      j["remote_port_id"] = static_cast<Json::UInt>(p.remotePortId.value());
    if(p.remoteSlotId.has_value())
      j["remote_slot_id"] = static_cast<Json::UInt>(p.remoteSlotId.value());
    if(p.remoteUbpuId.has_value())
      j["remote_ubpu_id"] = static_cast<Json::UInt>(p.remoteUbpuId.value());
    if(p.remoteIouId.has_value())
      j["remote_iou_id"] = static_cast<Json::UInt>(p.remoteIouId.value());
  };

void DataMapToObj(unordered_map<string, string> data_map, Node &obj);
void DataMapToObj(unordered_map<string, string> data_map, UbController &obj);
void DataMapToObj(unordered_map<string, string> data_map, Port &obj);
} // namespace topology::node
#endif
