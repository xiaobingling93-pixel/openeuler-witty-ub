#include "node_data_def.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace topology::node {
using namespace std;
unordered_map<string, ChipType> strToChipTypeMap = {
    {"CPU", ChipType::CPU},
    {"CPU-LINK", ChipType::CPULINK},
    {"NPU", ChipType::NPU},
    {"UNKNOWN", ChipType::UNKNOWN}};
unordered_map<ChipType, string> chipTypeToStrMap = {
    {ChipType::CPU, "CPU"},
    {ChipType::CPULINK, "CPU-LINK"},
    {ChipType::NPU, "NPU"},
    {ChipType::UNKNOWN, "UNKNOWN"}};
unordered_map<string, DieState> strToDieStateMap = {
    {"NORMAL", DieState::NORMAL},
    {"ABNORMAL", DieState::ABNORMAL},
    {"UNKNOWN", DieState::UNKNOWN}};
unordered_map<DieState, string> dieStateToStrMap = {
    {DieState::NORMAL, "NORMAL"},
    {DieState::ABNORMAL, "ABNORMAL"},
    {DieState::UNKNOWN, "UNKNOWN"}};
unordered_map<string, UbCState> strToUbCStateMap = {
    {"INITIAL", UbCState::INITIAL},   {"ONLINE", UbCState::ONLINE},
    {"OFFLINE", UbCState::OFFLINE},   {"RESETTING", UbCState::RESETTING},
    {"ABNORMAL", UbCState::ABNORMAL}, {"UNKNOWN", UbCState::UNKNOWN}};
unordered_map<UbCState, string> ubCStateToStrMap = {
    {UbCState::INITIAL, "INITIAL"},   {UbCState::ONLINE, "ONLINE"},
    {UbCState::OFFLINE, "OFFLINE"},   {UbCState::RESETTING, "RESETTING"},
    {UbCState::ABNORMAL, "ABNORMAL"}, {UbCState::UNKNOWN, "UNKNOWN"}};
unordered_map<string, PortState> strToPortStateMap = {
    {"UP", PortState::UP},
    {"DOWN", PortState::DOWN},
    {"UNKNOWN", PortState::UNKNOWN}};
unordered_map<PortState, string> portStateToStrMap = {
    {PortState::UP, "UP"},
    {PortState::DOWN, "DOWN"},
    {PortState::UNKNOWN, "UNKNOWN"}};
vector<string> GetHostIps(string str){
    vector<string> res;
    stringstream ss(str);
    string item;
    while (getline(ss, item, ':')) {
        res.push_back(item);
    }
    return res;
}

vector<uint32_t> GetPortIds(string str){
    vector<uint32_t> res;
    stringstream ss(str);
    string item;
    while (getline(ss, item, ':')) {
        res.push_back(static_cast<uint32_t>(stoi(item)));
    }
    return res;
}

string MergeStr(vector<string> strVec){
    if (strVec.empty()) {
        return "";
    }
    string res = strVec[0];
    for (int i = 1; i < strVec.size(); i++) {
        res += "," + strVec[i];
    }
    return res;
}

ChipType Str2ChipType(string chipType){
    if (strToChipTypeMap.find(chipType) == strToChipTypeMap.end()) {
        return ChipType::UNKNOWN;
    }
    return strToChipTypeMap[chipType];
}

string ChipType2Str(ChipType chipType){
    return chipTypeToStrMap[chipType];
}

DieState Str2DieState(string dieState){
    if (strToDieStateMap.find(dieState) == strToDieStateMap.end()) {
        return DieState::UNKNOWN;
    }
    return strToDieStateMap[dieState];
}

string DieState2Str(DieState dieState){
    return dieStateToStrMap[dieState];
}

UbCState Str2UbCState(string ubCState){
    if (strToUbCStateMap.find(ubCState) == strToUbCStateMap.end()) {
        return UbCState::UNKNOWN;
    }
    return strToUbCStateMap[ubCState];
}

string UbcState2Str(UbCState ubcState){
    return ubCStateToStrMap[ubcState];
}

PortState Str2PortState(string portState){
    if (strToPortStateMap.find(portState) == strToPortStateMap.end()) {
        return PortState::UNKNOWN;
    }
    return strToPortStateMap[portState];
}

string PortState2Str(PortState portState){
    return portStateToStrMap[portState];
}

void DataMapToObj(unordered_map<string, string> map, Node& obj){
    obj.deviceId = static_cast<uint32_t>(stoi(map["deviceId"]));
    obj.slotId = static_cast<uint32_t>(stoi(map["slotId"]));
    obj.hostname = map["hostname"];
    obj.ipAddrs = GetHostIps(map["ipAddrs"]);
    obj.chipNum = static_cast<uint32_t>(stoi(map["chipNum"]));
    obj.dieNum = static_cast<uint32_t>(stoi(map["dieNum"]));
    obj.chipType = Str2ChipType(map["chipType"]);
}

void DataMapToObj(unordered_map<string, string> map, UbController& obj){
    obj.dieGuid = map["dieGuid"];
    obj.ubcEid = map["ubcEid"];
    obj.deviceId = static_cast<uint32_t>(stoi(map["deviceId"]));
    obj.slotId = static_cast<uint32_t>(stoi(map["slotId"]));
    obj.chipId = static_cast<uint32_t>(stoi(map["chipId"]));
    obj.dieId = static_cast<uint32_t>(stoi(map["dieId"]));
    obj.primaryCna = map["primaryCna"];
    obj.portIds = GetPortIds(map["portIds"]);
    obj.dieState = Str2DieState(map["dieState"]);
    obj.ubcState = Str2UbCState(map["ubcState"]);
}

void DataMapToObj(unordered_map<string, string> map, Port& obj){
    obj.portId = static_cast<uint32_t>(stoi(map["portId"]));
    obj.portCna = map["portCna"];
    obj.primaryCna = map["primaryCna"];
    obj.deviceId = static_cast<uint32_t>(stoi(map["deviceId"]));
    obj.portState = Str2PortState(map["portState"]);
    //todo 这里解析暂时没有调用到，会有问题
    obj.remotePortId = static_cast<uint32_t>(stoi(map["remotePortIds"]));
    obj.remoteDeviceId = static_cast<uint32_t>(stoi(map["remoteDeviceId"]));
    obj.remoteSlotId = static_cast<uint32_t>(stoi(map["remoteSlotId"]));
    obj.remoteUbpuId = static_cast<uint32_t>(stoi(map["remoteUbpuId"]));
    obj.remoteIouId = static_cast<uint32_t>(stoi(map["remoteIouId"]));
}
} // namespace topology::node
