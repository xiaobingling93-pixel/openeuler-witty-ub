#ifndef LCNE_NODE_COLLECTOR_H
#define LCNE_NODE_COLLECTOR_H

#include <vector>
#include <memory>
#include <unordered_map>
#include "node_data_def.h"
#include "lcne_error.h"

namespace lcne::collector
{
    class LcneNodeCollector
    {
    public:
        LcneResult GetCurrNodeAllDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes,
                                         std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
                                         std::vector<std::shared_ptr<topology::node::Port>> &ports);
        LcneResult GetCurrNodeDeviceDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes);
        LcneResult GetCurrNodeUbCDataMap(std::vector<std::shared_ptr<topology::node::UbController>> &ubcs);
        LcneResult GetCurrNodePortDataMap(std::vector<std::shared_ptr<topology::node::Port>> &ports);
    };
}
#endif // LCNE_NODE_COLLECTOR_H