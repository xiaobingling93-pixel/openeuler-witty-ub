#define MODULE_NAME "LCNE"
#include "lcne_data_handler.h"
#include "http/rack_http_client.h"
#include "lcne_common.h"
#include "logger.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <unistd.h>
#include <unordered_set>

namespace lcne::handler
{
  LcneResult getXMLNodes(std::string request_path, std::map<lcne_key, xmlNode> &xml_nodes)
  {
    LcneResult ret = LCNE_FAIL;
    std::string response;
    ret = lcne::common::getHttpData(response, request_path);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "getXMLNodes-Error: getHttpData failed, result: " << ret;
      return ret;
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(response.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "getXMLNodes-Error: parse XML failed, error: "
                << doc.ErrorName();
      return ret;
    }
    tinyxml2::XMLElement *root_element = doc.RootElement();
    if (root_element == nullptr)
    {
      LOG_ERROR << "getXMLNodes-Error: root element is null";
      return ret;
    }
    tinyxml2::XMLElement *nodes_element =
        root_element->FirstChildElement("nodes");
    if (!nodes_element)
    {
      LOG_ERROR << "getXMLNodes-Error: nodes element is null";
      return ret;
    }
    for (tinyxml2::XMLElement *node_element =
             nodes_element->FirstChildElement("node");
         node_element != nullptr;
         node_element = node_element->NextSiblingElement("node"))
    {
      tinyxml2::XMLElement *slot_id_ele = node_element->FirstChildElement("slot");
      tinyxml2::XMLElement *chip_num_ele =
          node_element->FirstChildElement("ubpu");
      tinyxml2::XMLElement *die_num_ele = node_element->FirstChildElement("iou");
      tinyxml2::XMLElement *ubpu_type_ele =
          node_element->FirstChildElement("ubpu-type");
      tinyxml2::XMLElement *physical_ports_ele =
          node_element->FirstChildElement("physical-ports");
      std::unordered_map<uint32_t, xmlPhysicalPort> physical_ports;
      if (getNodePhysicalPorts(physical_ports_ele, physical_ports) == LCNE_FAIL)
      {
        LOG_ERROR << "getXMLNodes-Error: getNodePhysicalPorts failed";
        return ret;
      }
      uint32_t slot, ubpu, iou;
      if (lcne::common::convertTextToUint<uint32_t>(slot_id_ele, slot) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLNodes-Error: convert slot id to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(chip_num_ele, ubpu) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLNodes-Error: convert chip num to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(die_num_ele, iou) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLNodes-Error: convert die num to uint32 failed";
        return ret;
      }
      if (lcne::common::checkXML(ubpu_type_ele) == LCNE_FAIL)
      {
        LOG_ERROR << "getXMLNodes-Error: check ubpu type element failed";
        return ret;
      }
      std::string ubpu_type = ubpu_type_ele->GetText();
      lcne_key node_key = std::make_tuple(slot, ubpu, iou);
      xml_nodes[node_key] = xmlNode(slot, ubpu, iou, ubpu_type, physical_ports);
    }
    ret = LCNE_SUCCESS;
    return ret;
  }
  LcneResult getNodePhysicalPorts(
      tinyxml2::XMLElement *element,
      std::unordered_map<uint32_t, xmlPhysicalPort> &xml_physical_ports)
  {
    LcneResult ret = LCNE_FAIL;
    if (!element)
    {
      LOG_ERROR << "getNodePhysicalPorts-Error: physical ports element is null";
      return ret;
    }
    for (tinyxml2::XMLElement *physical_port_element =
             element->FirstChildElement("physical-port");
         physical_port_element != nullptr;
         physical_port_element =
             physical_port_element->NextSiblingElement("physical-port"))
    {
      tinyxml2::XMLElement *physical_port_id_ele =
          physical_port_element->FirstChildElement("physical-port-id");
      tinyxml2::XMLElement *physical_port_status_ele =
          physical_port_element->FirstChildElement("physical-port-status");
      tinyxml2::XMLElement *remote_slot_ele =
          physical_port_element->FirstChildElement("remote-slot");
      tinyxml2::XMLElement *remote_ubpu_ele =
          physical_port_element->FirstChildElement("remote-ubpu");
      tinyxml2::XMLElement *remote_iou_ele =
          physical_port_element->FirstChildElement("remote-iou");
      tinyxml2::XMLElement *remote_physical_port_id_ele =
          physical_port_element->FirstChildElement("remote-physical-port-id");
      uint32_t physical_port_id;
      std::optional<uint32_t> remote_slot, remote_ubpu, remote_iou,
          remote_physical_port_id;
      if (lcne::common::convertTextToUint<uint32_t>(
              physical_port_id_ele, physical_port_id) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getNodePhysicalPorts-Error: convert port num to uint32 failed";
        return ret;
      }
      if (lcne::common::checkXML(physical_port_status_ele) == LCNE_FAIL)
      {
        LOG_ERROR << "getNodePhysicalPorts-Error: check physical port status "
                     "element failed";
        return ret;
      }
      std::string physical_port_status = physical_port_status_ele->GetText();
      if (lcne::common::convertTextToOptionalUint<uint32_t>(remote_slot_ele,
                                                     remote_slot) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getNodePhysicalPorts-Error: convert remote slot to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToOptionalUint<uint32_t>(remote_ubpu_ele,
                                                     remote_ubpu) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getNodePhysicalPorts-Error: convert remote ubpu to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToOptionalUint<uint32_t>(remote_iou_ele,
                                                     remote_iou) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getNodePhysicalPorts-Error: convert remote iou to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToOptionalUint<uint32_t>(remote_physical_port_id_ele,
                                                     remote_physical_port_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getNodePhysicalPorts-Error: convert remote physical port "
                     "id to uint32 failed";
        return ret;
      }
      xml_physical_ports[physical_port_id] =
          xmlPhysicalPort(physical_port_id, physical_port_status, remote_slot,
                          remote_ubpu, remote_iou, remote_physical_port_id);
    }
    ret = LCNE_SUCCESS;
    return ret;
  }

  LcneResult getXMLIouInfo(std::string request_path,
                            std::map<lcne_key, xmlIouInfo> &xml_iou_infos)
  {
    LcneResult ret = LCNE_FAIL;
    std::string response;
    ret = lcne::common::getHttpData(response, request_path);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "getXMLIouInfos-Error: getHttpData failed, result: " << ret;
      return ret;
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(response.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "getXMLIouInfos-Error: parse XML failed, error: "
                << doc.ErrorName();
      return ret;
    }
    tinyxml2::XMLElement *root_element = doc.RootElement();
    if (!root_element)
    {
      LOG_ERROR << "getXMLIouInfos-Error: root element is null";
      return ret;
    }
    tinyxml2::XMLElement *iou_infos_element =
        root_element->FirstChildElement("iou-infos");
    if (!iou_infos_element)
    {
      LOG_ERROR << "getXMLIouInfos-Error: iou infos element is null";
      return ret;
    }
    for (tinyxml2::XMLElement *iou_info_element =
             iou_infos_element->FirstChildElement("iou-info");
         iou_info_element != nullptr;
         iou_info_element = iou_info_element->NextSiblingElement("iou-info"))
    {
      tinyxml2::XMLElement *guid_ele =
          iou_info_element->FirstChildElement("guid");
      tinyxml2::XMLElement *bus_controller_eid_ele =
          iou_info_element->FirstChildElement("bus-controller-eid");
      tinyxml2::XMLElement *slot_id_ele =
          iou_info_element->FirstChildElement("slot-id");
      tinyxml2::XMLElement *ubpu_id_ele =
          iou_info_element->FirstChildElement("ubpu-id");
      tinyxml2::XMLElement *iou_id_ele =
          iou_info_element->FirstChildElement("iou-id");
      tinyxml2::XMLElement *primary_cna_ele =
          iou_info_element->FirstChildElement("primary-cna");
      tinyxml2::XMLElement *iou_status_ele =
          iou_info_element->FirstChildElement("iou-status");
      if (lcne::common::checkXML(guid_ele) == LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: check guid element failed";
        return ret;
      }
      std::string guid = guid_ele->GetText();
      if (lcne::common::checkXML(bus_controller_eid_ele) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getXMLIouInfos-Error: check bus controller eid element failed";
        return ret;
      }
      std::string bus_controller_eid = bus_controller_eid_ele->GetText();
      uint32_t slot_id, ubpu_id, iou_id;
      if (lcne::common::convertTextToUint<uint32_t>(slot_id_ele, slot_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: convert slot id to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(ubpu_id_ele, ubpu_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: convert ubpu id to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(iou_id_ele, iou_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: convert iou id to uint32 failed";
        return ret;
      }
      if (lcne::common::checkXML(primary_cna_ele) == LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: check primary cna element failed";
        return ret;
      }
      std::string primary_cna = primary_cna_ele->GetText();
      if (lcne::common::checkXML(iou_status_ele) == LCNE_FAIL)
      {
        LOG_ERROR << "getXMLIouInfos-Error: check iou status element failed";
        return ret;
      }
      std::string iou_status = iou_status_ele->GetText();
      lcne_key iou_info_key = std::make_tuple(slot_id, ubpu_id, iou_id);
      xml_iou_infos[iou_info_key] =
          xmlIouInfo(guid, bus_controller_eid, slot_id, ubpu_id, iou_id,
                     primary_cna, iou_status);
    }
    ret = LCNE_SUCCESS;
    return ret;
  }

  LcneResult getXMLAddress(std::string request_path,
                           std::map<lcne_key, xmlAddress> &xml_addresses)
  {
    LcneResult ret = LCNE_FAIL;
    std::string response;
    ret = lcne::common::getHttpData(response, request_path);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "getXMLAddress-Error: getHttpData failed, result: " << ret;
      return ret;
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(response.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "getXMLAddress-Error: parse XML failed, error: "
                << doc.ErrorName();
      return ret;
    }
    tinyxml2::XMLElement *root_element = doc.RootElement();
    if (!root_element)
    {
      LOG_ERROR << "getXMLAddress-Error: root element is null";
      return ret;
    }
    tinyxml2::XMLElement *addresses_element =
        root_element->FirstChildElement("addresses");
    if (!addresses_element)
    {
      LOG_ERROR << "getXMLAddress-Error: addresses element is null";
      return ret;
    }
    for (tinyxml2::XMLElement *address_element =
             addresses_element->FirstChildElement("address");
         address_element != nullptr;
         address_element = address_element->NextSiblingElement("address"))
    {
      tinyxml2::XMLElement *slot_id_ele =
          address_element->FirstChildElement("slot");
      tinyxml2::XMLElement *ubpu_id_ele =
          address_element->FirstChildElement("ubpu");
      tinyxml2::XMLElement *iou_id_ele =
          address_element->FirstChildElement("iou");
      tinyxml2::XMLElement *bus_primary_cna_ele =
          address_element->FirstChildElement("bus-primary-cna");
      tinyxml2::XMLElement *physical_ports_ele =
          address_element->FirstChildElement("physical-ports");
      uint32_t slot_id, ubpu_id, iou_id;
      if (lcne::common::convertTextToUint<uint32_t>(slot_id_ele, slot_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLAddress-Error: convert slot id to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(ubpu_id_ele, ubpu_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLAddress-Error: convert ubpu id to uint32 failed";
        return ret;
      }
      if (lcne::common::convertTextToUint<uint32_t>(iou_id_ele, iou_id) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLAddress-Error: convert iou id to uint32 failed";
        return ret;
      }
      std::string bus_primary_cna = bus_primary_cna_ele->GetText();
      std::unordered_map<uint32_t, xmlAddressPhysicalPort> xml_physical_ports;
      if (getAddressPhysicalPorts(physical_ports_ele, xml_physical_ports) ==
          LCNE_FAIL)
      {
        LOG_ERROR << "getXMLAddress-Error: getAddressPhysicalPorts failed";
        return ret;
      }
      lcne_key address_key = std::make_tuple(slot_id, ubpu_id, iou_id);
      xml_addresses[address_key] = xmlAddress(
          slot_id, ubpu_id, iou_id, bus_primary_cna, xml_physical_ports);
    }
    ret = LCNE_SUCCESS;
    return ret;
  }

  LcneResult getAddressPhysicalPorts(
      tinyxml2::XMLElement *element,
      std::unordered_map<uint32_t, xmlAddressPhysicalPort> &xml_physical_ports)
  {
    LcneResult ret = LCNE_FAIL;
    if (!element)
    {
      LOG_ERROR
          << "getAddressPhysicalPorts-Error: physical ports element is null";
      return ret;
    }
    for (tinyxml2::XMLElement *physical_port_element =
             element->FirstChildElement("physical-port");
         physical_port_element != nullptr;
         physical_port_element =
             physical_port_element->NextSiblingElement("physical-port"))
    {
      tinyxml2::XMLElement *physical_port_id_ele =
          physical_port_element->FirstChildElement("physical-port-id");
      tinyxml2::XMLElement *port_cna_ele =
          physical_port_element->FirstChildElement("port-cna");
      tinyxml2::XMLElement *bus_port_cna_ele =
          physical_port_element->FirstChildElement("bus-port-cna");
      uint32_t physical_port_id;
      if (lcne::common::convertTextToUint<uint32_t>(
              physical_port_id_ele, physical_port_id) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getAddressPhysicalPorts-Error: convert port num to uint32 failed";
        return ret;
      }
      if (lcne::common::checkXML(port_cna_ele) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getAddressPhysicalPorts-Error: check port cna element failed";
        return ret;
      }
      std::string port_cna = port_cna_ele->GetText();
      if (lcne::common::checkXML(bus_port_cna_ele) == LCNE_FAIL)
      {
        LOG_ERROR
            << "getAddressPhysicalPorts-Error: check bus port cna element failed";
        return ret;
      }
      std::string bus_port_cna = bus_port_cna_ele->GetText();
      xml_physical_ports[physical_port_id] =
          xmlAddressPhysicalPort(physical_port_id, port_cna, bus_port_cna);
    }
    ret = LCNE_SUCCESS;
    return ret;
  }

  LcneResult getXMLLogicEntities(std::string request_path,
                                 std::shared_ptr<xmlLogicEntity> &xml_logic_entity)
  {
    // only one logic entity per node
    LcneResult ret = LCNE_FAIL;
    std::string response;
    ret = lcne::common::getHttpData(response, request_path);
    if (ret != LCNE_SUCCESS)
    {
      LOG_ERROR << "getXMLLogicEntities-Error: getHttpData failed, result: "
                << ret;
      return ret;
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(response.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "getXMLLogicEntities-Error: parse XML failed, error: "
                << doc.ErrorName();
      return ret;
    }
    tinyxml2::XMLElement *root_element = doc.RootElement();
    if (!root_element)
    {
      LOG_ERROR << "getXMLLogicEntities-Error: root element is null";
      return ret;
    }
    tinyxml2::XMLElement *logic_entity_element =
        root_element->FirstChildElement("logic-entities")->FirstChildElement("logic-entity");
    if (!logic_entity_element)
    {
      LOG_ERROR << "getXMLLogicEntities-Error: logic entity element is null";
      return ret;
    }
    tinyxml2::XMLElement *state_ele =
        logic_entity_element->FirstChildElement("state");
    if (lcne::common::checkXML(state_ele) == LCNE_FAIL)
    {
      LOG_ERROR << "getXMLLogicEntities-Error: check state element failed";
      return ret;
    }
    std::string state = state_ele->GetText();
    xml_logic_entity = std::make_shared<xmlLogicEntity>(state);
    ret = LCNE_SUCCESS;
    return ret;
  }
  LcneResult generateLcneNodes(const std::map<lcne_key, xmlNode> &xml_nodes, std::vector<std::shared_ptr<topology::node::Node>> &nodes)
  {

    for (const auto &xml_node_pair : xml_nodes)
    {
      const auto &xml_node = xml_node_pair.second;
      uint32_t device_id = xml_node.slot;
      uint32_t slot_id = xml_node.slot;
      uint32_t chip_num = xml_node.ubpu;
      uint32_t die_num = xml_node.iou;
      std::vector<std::string> ip_infos;
      std::string hostname = lcne::common::getHostname();
      topology::node::ChipType chip_type = topology::node::strToChipTypeMap[xml_node.ubpuType];
      auto node = std::make_shared<topology::node::Node>(device_id, slot_id, hostname, ip_infos, chip_num, die_num, chip_type);
      nodes.push_back(node);
    }
    return LCNE_SUCCESS;
  }

  LcneResult generateLcneUBController(const std::map<lcne_key, xmlNode> &xml_nodes,
                                      const std::map<lcne_key, xmlIouInfo> &xml_iou_infos, const std::shared_ptr<xmlLogicEntity> &xml_logic_entity,
                                      std::vector<std::shared_ptr<topology::node::UbController>> &ub_controllers)
  {
    for (const auto &[xml_iou_info_key, xml_iou_info] : xml_iou_infos)
    {
      std::string die_guid = xml_iou_info.guid;
      std::string ubc_eid = xml_iou_info.busControllerEid;
      uint32_t device_id = xml_iou_info.slotId;
      uint32_t slot_id = xml_iou_info.slotId;
      uint32_t chip_id = xml_iou_info.ubpuId;
      uint32_t iou_id = xml_iou_info.iouId;
      std::string primary_cna = xml_iou_info.primaryCna;

      auto it = xml_nodes.find(xml_iou_info_key);
      if (it == xml_nodes.end())
      {
        LOG_ERROR << "generateLcneUBController-Error: xml node not found";
        continue;
      }
      std::vector<uint32_t> port_ids;
      const xmlNode &node = it->second;
      for (const auto &[port_id, port] : node.physicalPorts)
      {
        port_ids.push_back(port_id);
      }

      std::string die_state_str = lcne::common::stringToUpper(xml_iou_info.iouStatus);
      auto die_state_it = topology::node::strToDieStateMap.find(die_state_str);
      if (die_state_it == topology::node::strToDieStateMap.end())
      {
        LOG_ERROR << "generateLcneUBController-Error: die state not found, state: " << die_state_str;
        return LCNE_FAIL;
      }
      topology::node::DieState die_state = die_state_it->second;

      std::string ubc_state_str = lcne::common::stringToUpper(xml_logic_entity->state);
      auto ubc_state_it = topology::node::strToUbCStateMap.find(ubc_state_str);
      if (ubc_state_it == topology::node::strToUbCStateMap.end())
      {
        LOG_ERROR << "generateLcneUBController-Error: ubc state not found, state: " << ubc_state_str;
        return LCNE_FAIL;
      }
      topology::node::UbCState ubc_state = ubc_state_it->second;

      auto ub_controller = std::make_shared<topology::node::UbController>(die_guid, ubc_eid, device_id, slot_id, chip_id, iou_id,
                                                          primary_cna, port_ids, die_state, ubc_state);
      ub_controllers.push_back(ub_controller);
    }
    return LCNE_SUCCESS;
  }

  LcneResult generateLcnePort(const std::map<lcne_key, xmlNode> &xml_nodes,
                              const std::map<lcne_key, xmlAddress> &xml_addresses, std::vector<std::shared_ptr<topology::node::Port>> &ports)
  {
    for (const auto &[xml_address_key, xml_address] : xml_addresses)
    {
      for (const auto &[port_id, physical_port] : xml_address.physicalPorts)
      {
        uint32_t physical_port_id = physical_port.physicalPortId;
        std::string port_cna = physical_port.portCna;
        std::string primary_cna = xml_address.primaryCna;
        uint32_t device_id = std::get<0>(xml_address_key);
        auto nodes_it = xml_nodes.find(xml_address_key);
        if (nodes_it == xml_nodes.end())
        {
          LOG_ERROR << "generateLcnePort-Error: xml node not found";
          continue;
        }
        xmlNode node = nodes_it->second;
        auto node_port_it = node.physicalPorts.find(physical_port_id);
        if (node_port_it == node.physicalPorts.end())
        {
          LOG_ERROR << "generateLcnePort-Error: xml physical port not found, key: " << port_id;
          continue;
        }
        xmlPhysicalPort node_physical_port = node_port_it->second;
        std::string port_state_str = lcne::common::stringToUpper(node_physical_port.physicalPortStatus);
        auto port_state_it = topology::node::strToPortStateMap.find(port_state_str);
        if (port_state_it == topology::node::strToPortStateMap.end())
        {
          LOG_ERROR << "generateLcnePort-Error: port state not found, state: " << port_state_str;
          return LCNE_FAIL;
        }
        topology::node::PortState port_state = port_state_it->second;
        std::optional<uint32_t> remote_slot = node_physical_port.remoteSlot;
        std::optional<uint32_t> remote_ubpu = node_physical_port.remoteUbpu;
        std::optional<uint32_t> remote_iou = node_physical_port.remoteIou;
        std::optional<uint32_t> remote_physical_port_id = node_physical_port.remotePhysicalPortId;
        auto port = std::make_shared<topology::node::Port>(physical_port_id, port_cna, primary_cna, device_id, port_state,
                                           remote_physical_port_id, remote_slot, remote_slot, remote_ubpu, remote_iou);
        ports.push_back(port);
      }
    }
    return LCNE_SUCCESS;
  }
} // namespace lcne::handler
