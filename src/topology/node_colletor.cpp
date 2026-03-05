#define MODULE_NAME "NODE_COLLECTOR"
#include <memory>
#include <vector>
#include <tuple>
#include <string>
#include "node_collector.h"
#include "database.h"
#include "logger.h"

namespace topology::node {
using namespace database;
OP_RET NodeCollector::InitDb(std::shared_ptr<Database> db_)
{
    db = db_;
    LOG_INFO << "Init database";
    return SUCCESS;
}
OP_RET NodeCollector::StartDb()
{
    OP_RET ret = db->CreateTable("Device",
        vector<tuple<string, string, bool, bool>>({make_tuple("deviceId", "TEXT", true, true),
            make_tuple("slotId", "TEXT", true, false), make_tuple("hostname", "TEXT", false, false),
            make_tuple("ipAddrs", "TEXT", false, false), make_tuple("chipNum", "TEXT", true, false),
            make_tuple("dieNum", "TEXT", true, false), make_tuple("chipType", "TEXT", false, false)}));
        if (ret != SUCCESS) {
            return ret;
        }
        ret = db->CreateTable("UbController",
            vector<tuple<string, string, bool, bool>>(
                {make_tuple("dieGuid", "TEXT", true, false), make_tuple("ubcEid", "TEXT", true, false),
                    make_tuple("deviceId", "TEXT", true, false), make_tuple("slotId", "TEXT", true, false),
                    make_tuple("chipId", "TEXT", true, false), make_tuple("dieId", "TEXT", true, false),
                    make_tuple("primaryCna", "TEXT", true, true), make_tuple("portIds", "TEXT", true, false),
                    make_tuple("dieState", "TEXT", true, false), make_tuple("ubcState", "TEXT", true, false)}));
        if (ret != SUCCESS) {
            return ret;
        }
        ret = db->CreateTable("Port",
            vector<tuple<string, string, bool, bool>>(
                {make_tuple("portId", "TEXT", true, true), make_tuple("portCna", "TEXT", true, false),
                    make_tuple("primaryCna", "TEXT", true, true), make_tuple("deviceId", "TEXT", true, false),
                    make_tuple("portState", "TEXT", true, false), make_tuple("remotePortId", "TEXT", true, false),
                    make_tuple("remoteDeviceId", "TEXT", true, false), make_tuple("remoteSlotId", "TEXT", true, false),
                    make_tuple("remoteUbpuId", "TEXT", true, false), make_tuple("remoteIouId", "TEXT", true, false)}));
        if (ret != SUCCESS) {
            return ret;
        }
        return SUCCESS;
}
shared_ptr<Database> NodeCollector::GetDb()
{
    return db;
}
OP_RET NodeCollector::InsertDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    OP_RET res = OP_RET::SUCCESS;
    for (auto device : nodes) {
        unordered_map<string, string> data_map = device->ObjToDataMap();
        OP_RET ret = db->InsertData("Device", data_map);
        if (ret != OP_RET::SUCCESS) {
            res = ret;
        }
    }
    return res;
}
OP_RET NodeCollector::InsertUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs)
{
    OP_RET res = OP_RET::SUCCESS;
    for (auto ubc : ubcs) {
        unordered_map<string, string> data_map = ubc->ObjToDataMap();
        OP_RET ret = db->InsertData("UbController", data_map);
        if (ret != OP_RET::SUCCESS) {
            res = ret;
        }
    }
    return res;
}
OP_RET NodeCollector::InsertPortData(vector<std::shared_ptr<topology::node::Port>> &ports)
{
    OP_RET res = OP_RET::SUCCESS;
    for (auto port : ports) {
        unordered_map<string, string> data_map = port->ObjToDataMap();
        OP_RET ret = db->InsertData("Port", data_map);
        if (ret != OP_RET::SUCCESS) {
            res = ret;
        }
    }
    return res;
}

OP_RET NodeCollector::QueryHisNodeData(
    vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;
    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryHisData("Device", data, &res, 0, database::GetNowTimestamp());
    for (auto data_map : res) {
        topology::node::Node device;
        topology::node::DataMapToObj(data_map, device);
        nodes.push_back(std::make_shared<Node>(device.deviceId, decvice.slotId, device.hostname, device.ipAddrs,
            device.chipNum, device.dieNum, deice.chipType));
    }
    return ret;
}
OP_RET NodeCollector::QueryHisUbCData(
    vector<std::shared_ptr<topology::node::UbController>> &ubcs)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;
    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryHisData("UbController", data, &res, 0, database::GetNowTimestamp());
    for (auto data_map : res) {
        topology::node::UbController ubc;
        topology::node::DataMapToObj(data_map, ubc);
        ubcs.push_back(std::make_shared<UbController>(ubc.dieGuid, ubc.ubcEid, ubc.deviceId, ubc.slotId, ubc.chipId,
            ubc.dieId, ubc.primaryCna, ubc.portIds, ubc.dieState, ubc.ubcState));
    }
    return ret;
}
OP_RET NodeCollector::QueryHisPortData(
    vector<std::shared_ptr<topology::node::Port>> &ports)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;

    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryHisData("Port", data, &res, 0, database::GetNowTimestamp());
    for (auto data_map : res) {
        topology::node::Port port;
        topology::node::DataMapToObj(data_map, port);
        ports.push_back(std::make_shared<Port>(port.portId, port.portCna, port.primaryCna, port.deviceId, port.portState,
            port.remotePortId, port.remoteDeviceId, port.remoteSlotId, port.remoteUbpuId, port.remoteIouId));
    }
    return ret;
}

OP_RET NodeCollector::QueryCurDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;
    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryCurrData("Device", data, &res);
    for (auto data_map : res) {
        topology::node::Node device;
        topology::node::DataMapToObj(data_map, device);
        nodes.push_back(std::make_shared<Node>(device.deviceId, decvice.slotId, device.hostname, device.ipAddrs,
            device.chipNum, device.dieNum, deice.chipType));
    }
    return ret;
}
OP_RET NodeCollector::QueryCurUbCData(
    vector<std::shared_ptr<topology::node::UbController>> &ubcs)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;
    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryCurrData("UbController", data, &res);
    for (auto data_map : res) {
        topology::node::UbController ubc;
        topology::node::DataMapToObj(data_map, ubc);
        ubcs.push_back(std::make_shared<UbController>(ubc.dieGuid, ubc.ubcEid, ubc.deviceId, ubc.slotId, ubc.chipId,
            ubc.dieId, ubc.primaryCna, ubc.portIds, ubc.dieState, ubc.ubcState));
    }
    return ret;
}
OP_RET NodeCollector::QueryCurPortData(vector<std::shared_ptr<topology::node::Port>> &ports)
{
    std::unordered_map<std::string, std::pair<database::COMP_SYMB, std::string>> data;
    std::vector<std::unordered_map<std::string, std::string>> res;
    OP_RET ret = db->QueryCurrData("Port", data, &res);
    for (auto data_map : res) {
        topology::node::Port port;
        topology::node::DataMapToObj(data_map, port);
        ports.push_back(std::make_shared<Port>(port.portId, port.portCna, port.primaryCna, port.deviceId, port.portState,
            port.remotePortId, port.remoteDeviceId, port.remoteSlotId, port.remoteUbpuId, port.remoteIouId));
    }
    return ret;
}
} // namespace topology::node