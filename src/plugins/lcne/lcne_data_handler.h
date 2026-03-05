#ifndef LCNE_DATA_HANDLER_H
#define LCNE_DATA_HANDLER_H
#include <tinyxml2.h>
#include <optional>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include "lcne_error.h"
#include "node_data_def.h"

namespace lcne::handler
{
    // lcne key is  (slot, ubpu, iou)
    using lcne_key = std::tuple<uint32_t, uint32_t, uint32_t>;

    struct xmlPhysicalPort
    {
        uint32_t physicalPortId;
        std::string physicalPortStatus;
        std::optional<uint32_t> remoteSlot;
        std::optional<uint32_t> remoteUbpu;
        std::optional<uint32_t> remoteIou;
        std::optional<uint32_t> remotePhysicalPortId;

        xmlPhysicalPort() = default;

        xmlPhysicalPort(uint32_t physicalPortId, const std::string &physicalPortStatus,
                        std::optional<uint32_t> remoteSlot, std::optional<uint32_t> remoteUbpu,
                        std::optional<uint32_t> remoteIou, std::optional<uint32_t> remotePhysicalPortId)
            : physicalPortId(physicalPortId), physicalPortStatus(physicalPortStatus),
              remoteSlot(remoteSlot), remoteUbpu(remoteUbpu),
              remoteIou(remoteIou), remotePhysicalPortId(remotePhysicalPortId) {}
    };

    struct xmlNode
    {
        uint32_t slot;
        uint32_t ubpu;
        uint32_t iou;
        std::string ubpuType;
        std::unordered_map<uint32_t, xmlPhysicalPort> physicalPorts;

        xmlNode() = default;
        xmlNode(uint32_t slot, uint32_t ubpu, uint32_t iou, std::string &ubpuType,
                std::unordered_map<uint32_t, xmlPhysicalPort> physicalPorts)
            : slot(slot), ubpu(ubpu), iou(iou), ubpuType(ubpuType), physicalPorts(std::move(physicalPorts)) {}
    };

    struct xmlIouInfo
    {
        string guid;
        string busControllerEid;
        uint32_t slotId;
        uint32_t ubpuId;
        uint32_t iouId;
        std::string primaryCna;
        std::string iouStatus;

        xmlIouInfo() = default;
        xmlIouInfo(const std::string &guid, const std::string &busControllerEid, uint32_t slotId, uint32_t ubpuId, uint32_t iouId,
                   const std::string &primaryCna, const std::string &iouStatus)
            : guid(guid), busControllerEid(busControllerEid), slotId(slotId), ubpuId(ubpuId), iouId(iouId),
              primaryCna(primaryCna), iouStatus(iouStatus) {}
    };

    struct xmlAddressPhysicalPort
    {
        uint32_t physicalPortId;
        std::string portCna;
        std::string busPortCna;

        xmlAddressPhysicalPort() = default;
        xmlAddressPhysicalPort(uint32_t physicalPortId, const std::string &portCna, const std::string &busPortCna)
            : physicalPortId(physicalPortId), portCna(portCna), busPortCna(busPortCna) {}
    };

    struct xmlAddress
    {
        uint32_t slotId;
        uint32_t ubpuId;
        uint32_t iouId;
        std::string primaryCna;
        std::vector<xmlAddressPhysicalPort> physicalPorts;

        xmlAddress() = default;
        xmlAddress(uint32_t slotId, uint32_t ubpuId, uint32_t iouId, const std::string &primaryCna,
                   std::vector<xmlAddressPhysicalPort> physicalPorts)
            : slotId(slotId), ubpuId(ubpuId), iouId(iouId), primaryCna(primaryCna),
              physicalPorts(std::move(physicalPorts)) {}
    };

    struct xmlLogicEntity
    {
        std::string state;
        xmlLogicEntity() = default;
        xmlLogicEntity(const std::string &state) : state(state) {}
    };

    LcneResult getXMLNodes(string request_path, map<lcne_key, xmlNode> &xml_nodes);
    LcneResult getNodePhysicalPorts(tinyxml2::XMLElement *element,
                                    std::unordered_map<uint32_t, xmlPhysicalPort> &xml_physical_ports);
    LcneResult getXMLIouInfo(string request_path, map<lcne_key, xmlIouInfo> &xml_iou_infos);
    LcneResult getXMLAddress(string request_path, map<lcne_key, xmlAddress> &xml_addresses);
    LcneResult getAddressPhysicalPorts(tinyxml2::XMLElement *element,
                                       std::unordered_map<uint32_t, xmlAddressPhysicalPort> &xml_address_physical_ports);
    LcneResult getXMLLogicEntities(string request_path, shared_ptr<xmlLogicEntity> &xml_logic_entity);
    LcneResult generateLcneNodes(const map<lcne_key, xmlNode> &xml_nodes, vector<shared_ptr<Node>> &nodes);
    LcneResult generateLcneUBController(
        const map<lcne_key, xmlNode> &xml_nodes,
        const map<lcne_key, xmlIouInfo> &xml_iou_infos,
        const shared_ptr<xmlLogicEntity> &xml_logic_entity,
        vector<shared_ptr<UbController>> &ub_controllers);
    LcneResult generateLcnePort(
        const map<lcne_key, xmlNode> &xml_nodes,
        const map<lcne_key, xmlAddress> &xml_addresses,
        vector<shared_ptr<Port>> &ports);
} // namespace lcne::handler
#endif // LCNE_DATA_HANDLER_H
