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

#ifndef LCNE_COMMON_H
#define LCNE_COMMON_H
#include "lcne_error.h"
#include "logger.h"
#include "node_data_def.h"
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <tinyxml2.h>

namespace lcne::common
{
  using namespace topology::node;
  constexpr const char *LCNE_PORT = "34256";
  constexpr const char *LCNE_URL = "http://127.0.0.1";
  constexpr const char *LCNE_LOCAL_IP = "127.0.0.1";
  constexpr const char *LCNE_CONTENT_TYPE = "application/yang-data+xml";
  constexpr const char *LCNE_NODES_REQ_PATH =
      "/restconf/data/huawei-lingqu-topology:lingqu-topology/nodes";
  constexpr const char *LCNE_ADDRESS_REQ_PATH =
      "/restconf/data/huawei-lingqu-topology:lingqu-topology/addresses";
  constexpr const char *LCNE_IOU_INFOS_REQ_PATH =
      "/restconf/data/huawei-vbussw-service:vbussw-service/iou-infos";
  constexpr const char *LCNE_LOGIC_ENTITIES_REQ_PATH =
      "/restconf/data/huawei-vbussw-inventory:vbussw-inventory/logic-entities";
  constexpr const char *LCNE_NOTIFY_LINK_REQ_PATH =
      "/restconf/operations/notifications:create-subscription";
  constexpr const char *LCNE_NOTIFY_TOPO_PATH = "/topolink/change";
  constexpr const char *LCNE_TOPO_INIT_PATH = "/topology/init";
  constexpr const char *LCNE_TOPO_CHANGE_PATH = "/topology/change";
  constexpr const char *CONFIG_PATH = "/etc/witty-ub";
  constexpr const char *CONFIG_FILE = "config.xml";
  constexpr const char *JSON_OUTPUT_FILE = "/var/witty-ub/lcne-topology.json";
  constexpr mode_t CONFIG_FILE_PERM_755 = 0755; // Todo
  constexpr mode_t CONFIG_PATH_PERM_755 = 0755;
  constexpr mode_t JSON_OUTPUT_FILE_PERM_640 = 0640;

  template <typename... Args>
  tinyxml2::XMLElement *getElement(string &xml_content, Args &&...args)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError result = doc.Parse(xml_content.c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "getElement-Error: parse xml failed, error code: " << result;
      return nullptr;
    }

    tinyxml2::XMLElement *current = doc.RootElement();
    if (!current)
    {
      LOG_ERROR << "getElement-Error: get root element failed1";
      return nullptr;
    }
    std::vector<std::string> path = {args...};
    for (const auto &name : path)
    {
      if (!current)
      {
        LOG_ERROR << "getElement-Error: get element failed, path: " << name;
        break;
      }
      current = current->FirstChildElement(name.c_str());
    }
    return current;
  }

  std::string getHostname();
  LcneResult getNodeIpInfos(std::vector<std::string> &ipInfos);
  bool isSpecialIp(const std::string &ip);

  LcneResult checkXML(tinyxml2::XMLElement *element);

  template <typename T>
  LcneResult convertTextToUint(tinyxml2::XMLElement *element, T &result)
  {
    static_assert(std::is_unsigned_v<T>, "T must be unsigned integer type");
    LcneResult ret = LCNE_FAIL;
    if (checkXML(element) == LCNE_FAIL)
    {
      LOG_ERROR << "convertTextToUint-Error: check xml failed, path: "
                << element->Name();
      return ret;
    }
    const char *text = element->GetText();

    try
    {
      unsigned long value = std::stoul(std::string(text));
      if (value > std::numeric_limits<T>::max())
      {
        LOG_ERROR << "convertTextToUint-Error: value out of range, path: "
                  << element->Name();
        return ret;
      }
      ret = LCNE_SUCCESS;
      result = static_cast<T>(value);
      return ret;
    }
    catch (const std::invalid_argument &e)
    {
      LOG_ERROR << "convertTextToUint-Error: convert text to uint failed, path: "
                << element->Name() << ", error: " << e.what();
      return ret;
    }
    catch (const std::out_of_range &e)
    {
      LOG_ERROR << "convertTextToUint-Error: convert text to uint failed, path: "
                << element->Name() << ", error: " << e.what();
      return ret;
    }
  }

  template <typename T>
  LcneResult convertTextToOptionalUint(tinyxml2::XMLElement *element,
                                       std::optional<T> &result)
  {
    static_assert(std::is_unsigned_v<T>, "T must be unsigned integer type");
    LcneResult ret = LCNE_FAIL;
    if (checkXML(element) == LCNE_FAIL)
    {
      LOG_ERROR << "convertTextToOptionalUint-Error: check xml failed, path: "
                << element->Name();
      return ret;
    }
    const char *text = element->GetText();
    try
    {
      unsigned long value = std::stoul(std::string(text));
      if (value > std::numeric_limits<T>::max())
      {
        LOG_ERROR << "convertTextToOptionalUint-Error: value out of range, path: "
                  << element->Name();
        return ret;
      }
      ret = LCNE_SUCCESS;
      result = static_cast<T>(value);
      return ret;
    }
    catch (const std::invalid_argument &e)
    {
      result = std::nullopt;
      ret = LCNE_SUCCESS;
      return ret;
    }
    catch (const std::out_of_range &e)
    {
      LOG_ERROR << "convertTextToOptionalUint-Error: convert text to uint failed, path: "
                << element->Name() << ", error: " << e.what();
      return ret;
    }
  }
  LcneResult getTextAsString(tinyxml2::XMLElement *element, std::string &str);
  std::string stringToUpper(const std::string &s);
  LcneResult getHttpData(std::string &resp_body, std::string req_path);
  LcneResult postLinkInfoNotify();
  LcneResult SaveIpToConfigFile(std::string master_ip);
} // namespace lcne::common
#endif
