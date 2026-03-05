#ifndef NODE_COLLECTOR_H
#define NODE_COLLECTOR_H
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "node_data_def.h"
#include "rack_module.h"
#include "ubse_context.h"
#include "database.h"

namespace topology::node {
using namespace ubse::context;
using namespace database;
class NodeCollector {
public:
    OP_RET InitDb(shared_ptr<Database> db_);
    OP_RET StartDb();
    shared_ptr<Database> GetDb();
    OP_RET InsertDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    OP_RET InsertUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs);
    OP_RET InsertPortData(vector<std::shared_ptr<topology::node::Port>> &ports);
    OP_RET QueryHisNodeData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    OP_RET QueryHisUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs);
    OP_RET QueryHisPortData(vector<std::shared_ptr<topology::node::Port>> &ports);
    OP_RET QueryCurDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    OP_RET QueryCurUbCData(vector<std::shared_ptr<topology::node::UbController> &ubcs);
    OP_RET QueryCurPortData(vector<std::shared_ptr<topology::node::Port>> &ports);

private:
    unordered_map<uint32_t, Node> deviceInfos; // deviceId, device
    unordered_map<uint32_t, vector<UbController>> UbControllerInfos; // deviceId, ubControllers
    unordered_map<uint16_t, vector<Port>> PortInfos; // primaryCna, ports
    shared_ptr<Database> db;
};
} // namespace topology::node
#endif // NODE_COLLECTOR_H