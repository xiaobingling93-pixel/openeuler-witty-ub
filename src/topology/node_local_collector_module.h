#ifndef NODE_LOCAL_COLLECTOR_MODULE_H
#define NODE_LOCAL_COLLECTOR_MODULE_H
#include <string>
#include <vector>
#include <memory>
#include "rack_error.h"
#include "rack_module.h"
#include "database_module.h"
#include "node_collector.h"
#inlcude "node_data_def.h"

namespace topology::node {
using namespace rack::module;
using namespace database;
class NodeLocalCollectorModule : public RackModule {
public:
    NodeLocalCollectorModule() { dependencies.push_back(typeid(DatabaseModule)); }
    ~NodeLocalCollectorModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
    shared_ptr<NodeCollector> GetCollector() { return collector; }
    RackResult InsertDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    RackResult InsertUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs);
    RackResult InsertPortData(vector<std::shared_ptr<topology::node::Port>> &ports);
    RackResult QueryCurDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    RackResult QueryHisDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes);
    RackResult QueryCurUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs);
    RackResult QueryHisUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs);
    RackResult QueryCurPortData(vector<std::shared_ptr<topology::node::Port>> &ports);
    RackResult QueryHisPortData(vector<std::shared_ptr<topology::node::Port>> &ports);

private:
    shared_ptr<NodeCollector> collector;
};
} // namespace topology::node
#endif //NODE_LOCAL_COLLECTOR_MODULE_H