#define MODULE_NAME "LCNE"
#include "lcne_node_collector.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "lcne_common.h"
#include "lcne_data_handler.h"
namespace lcne::collector
{
    LcneResult LcneNodeCollector::GetCurrNodeAllDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes,
                                                        std::vector<std::shared_ptr<topology::node::UbController>> &ubcs,
                                                        std::vector<std::shared_ptr<topology::node::Port>> &ports)
    {
        LcneResult ret = GetCurrNodeDeviceDataMap(nodes);
        ret = GetCurrNodeUbCDataMap(ubcs);
        ret = GetCurrNodePortDataMap(ports);
        return ret;
    }
    LcneResult LcneNodeCollector::GetCurrNodeDeviceDataMap(std::vector<std::shared_ptr<topology::node::Node>> &nodes)
    {
        LcneResult ret = LCNE_SUCCESS;
        std::map <lcne::handler::lcne_key, lcne::handler::xmlNode> xml_nodes;
        ret = lcne::handler::getXMLNodes(lcne::common::LCNE_NODES_REQ_PATH, xml_nodes);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeDeviceDataMap-Error: failed to get node xml data";
            return ret;
        }
        ret = lcne::handler::generateLcneNodes(xml_nodes, nodes);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeDeviceDataMap-Error: failed to generate lcne nodes";
            return ret;
        }
        return ret;
    }
    LcneResult LcneNodeCollector::GetCurrNodeUbCDataMap(std::vector<std::shared_ptr<topology::node::UbController>> &ubcs)
    {
        LcneResult ret = LCNE_SUCCESS;
        std::map<lcne::handler::lcne_key, lcne::handler::xmlNode> xml_nodes;
        std::map<lcne::handler::lcne_key, lcne::handler::xmlIouInfo> xml_iou_infos;
        std::shared_ptr<lcne::handler::xmlLogicEntity> xml_logic_entities;
        ret = lcne::handler::getXMLNodes(lcne::common::LCNE_NODES_REQ_PATH, xml_nodes);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeUbCDataMap-Error: failed to get ubc xml data";
            return ret;
        }
        ret = lcne::handler::getXMLIouInfo(lcne::common::LCNE_IOU_INFOS_REQ_PATH, xml_iou_infos);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeUbCDataMap-Error: failed to get iou info xml data";
            return ret;
        }
        ret = lcne::handler::getXMLLogicEntities(lcne::common::LCNE_LOGIC_ENTITIES_REQ_PATH, xml_logic_entities);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeUbCDataMap-Error: failed to get logic entity xml data";
            return ret;
        }

        ret = lcne::handler::generateLcneUBController(xml_nodes, xml_iou_infos, xml_logic_entities, ubcs);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodeUbCDataMap-Error: failed to generate lcne ubcs";
            return ret;
        }
        return ret;
    }
    LcneResult LcneNodeCollector::GetCurrNodePortDataMap(std::vector<std::shared_ptr<topology::node::Port>> &ports)
    {
        LcneResult ret = LCNE_SUCCESS;
        std::map<lcne::handler::lcne_key, lcne::handler::xmlNode> xml_nodes;
        std::map<lcne::handler::lcne_key, lcne::handler::xmlAddress> xml_addresses;
        ret = lcne::handler::getXMLNodes(lcne::common::LCNE_NODES_REQ_PATH, xml_nodes);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodePortDataMap-Error: failed to get port xml data";
            return ret;
        }
        ret = lcne::handler::getXMLAddress(lcne::common::LCNE_ADDRESS_REQ_PATH, xml_addresses);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodePortDataMap-Error: failed to get address xml data";
            return ret;
        }
        ret = lcne::handler::generateLcnePort(xml_nodes, xml_addresses, ports);
        if (ret != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneNodeCollector::GetCurrNodePortDataMap-Error: failed to generate lcne ports";
            return ret;
        }
        return ret;
    }
} // namespace lcne::collector
