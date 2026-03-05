#define MODULE_NAME "NODE-LOCAL-COLLECTOR"
#include <tuple>
#include <typeindex>
#include "node_local_collector_module.h"
#include "rack_error.h"
#include "database_module.h"
#include "ubse_context.h"

namespace topology::node {
using namespace ubse::context;
using namespace database;
RackResult NodeLocalCollectorModule::Initialize()
{
    collector = std::make_shared<NodeCollector>();
    collector->InitDb(UbseContext::GetInstance().GetModule<DatabaseModule>()->GetDatabase());
    return RACK_OK;
}
void NodeLocalCollectorModule::UnInitialize()
{
    return;
}
RackResult NodeLocalCollectorModule::Start()
{
    collector->StartDb();
    LOG_INFO << "NodeLocalCollectorModule Start Success";
    return RACK_OK;
}
void NodeLocalCollectorModule::Stop()
{
    return;
}
RackResult NodeLocalCollectorModule::InsertDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    RackResult ret = collector->InsertDeviceData(nodes);
    return ret;
}
RackResult NodeLocalCollectorModule::InsertUbCData(vector<std::shared_ptr<topology::node::UbControllers>> &ubcs)
{
    RackResult ret = collector->InsertUbCData(ubcs);
    return ret;
}
RackResult NodeLocalCollectorModule::InsertPortData(vector<std::shared_ptr<topology::node::Port>> &ports)
{
    RackResult ret = collector->InsertPortData(ports);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryCurDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    RackResult ret = collector->QueryCurDeviceData(nodes);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryHisDeviceData(vector<std::shared_ptr<topology::node::Node>> &nodes)
{
    RackResult ret = collector->QueryHisNodeData(nodes);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryCurUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs)
{
    RackResult ret = collector->QueryCurUbCData(ubcs);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryHisUbCData(vector<std::shared_ptr<topology::node::UbController>> &ubcs)
{
    RackResult ret = collector->QueryHisUbCData(ubcs);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryCurPortData(vector<std::shared_ptr<topology::node::Port>> &ports)
{
    RackResult ret = collector->QueryCurPortData(ports);
    return ret;
}
RackResult NodeLocalCollectorModule::QueryHisPortData(vector<std::shared_ptr<topology::node::Port>> &ports)
{
    RackResult ret = collector->QueryHisPortData(ports);
    return ret;
}
} // namespace topology::node