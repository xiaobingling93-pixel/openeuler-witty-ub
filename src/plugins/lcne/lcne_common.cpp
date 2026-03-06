#define MODULE_NAME "LCNE"
#include "lcne_common.h"
#include "http/rack_http_client.h"
#include "http/rack_http_types.h"
#include "logger.h"
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <tinyxml2.h>
#include <unistd.h>
#include <unordered_set>

namespace lcne::common
{
  std::string getHostname()
  {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
      return std::string(hostname);
    }
    else
    {
      LOG_ERROR << "getHostname-Error: get hostname failed";
    }
    return "";
  }

  LcneResult getNodeIpInfos(std::vector<std::string> &ipInfos)
  {
    LcneResult ret = LCNE_FAIL;
    std::ifstream file("/proc/net/fib_trie");
    if (!file.is_open())
    {
      LOG_ERROR << "getNodeIpInfos-Error: open /proc/net/fib_trie failed";
      return ret;
    }

    std::string line;
    std::unordered_set<std::string> uniqueIPs;
    std::regex ipRegex(R"(\b(\d+\.\d+\.\d+\.\d+)\b)");

    while (std::getline(file, line))
    {
      std::smatch match;
      if (std::regex_search(line, match, ipRegex))
      {
        std::string ip = match.str(1);
        if (!isSpecialIp(ip))
        {
          uniqueIPs.insert(ip);
        }
      }
    }

    file.close();
    ipInfos.insert(ipInfos.end(), uniqueIPs.begin(), uniqueIPs.end());
    ret = LCNE_SUCCESS;
    return ret;
  }

  bool isSpecialIp(const std::string &ip)
  {
    return std::regex_match(ip,
                            std::regex(R"(^0\.0\.0\.0$|127\..*|169\.254\..*)$)"));
  }

  LcneResult getTextAsString(tinyxml2::XMLElement *element, std::string &str)
  {
    if (checkXML(element) == LCNE_FAIL)
    {
      LOG_ERROR << "getTextAsString-Error: element is null";
      return LCNE_FAIL;
    }
    str = std::string(element->GetText());
    return LCNE_SUCCESS;
  }

  LcneResult checkXML(tinyxml2::XMLElement *element)
  {
    if (!element)
    {
      LOG_ERROR << "checkXML-Error: element is null";
      return LCNE_FAIL;
    }
    const char *text = element->GetText();
    if (!text)
    {
      LOG_ERROR << "checkXML-Error: text is null";
      return LCNE_FAIL;
    }

    if (strlen(text) == 0)
    {
      LOG_ERROR << "checkXML-Error: text is empty";
      return LCNE_FAIL;
    }
    return LCNE_SUCCESS;
  }

  std::string stringToUpper(const std::string &s)
  {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });
    return r;
  }

  LcneResult getHttpData(std::string &resp_body, std::string req_path)
  {
    rack::com::RackHttpClient client(std::string(LCNE_URL) + ":" +
                                     std::string(LCNE_PORT));
    rack::com::RackComContext ctx;
    ctx.metadata["Content-Type"] = LCNE_CONTENT_TYPE;
    rack::com::RackHttpRequest req;
    req.method = rack::com::RackHttpMethod::GET;
    req.path = std::string(req_path);
    LOG_INFO << "getHttpData-Info: send request to "
             << std::string(LCNE_URL) + ":" + std::string(LCNE_PORT) + req_path;
    auto res = client.Do(ctx, req);
    if (res.Ok())
    {
      const auto &resp = res.value;
      if (resp->body.empty())
      {
        LOG_ERROR << "getHttpData-Error: response body is empty";
        return LCNE_FAIL;
      }
      resp_body = resp->body;
      LOG_DEBUG << "getHttpData-Debug: response body is " << resp_body;
      return LCNE_SUCCESS;
    }
    LOG_ERROR
        << "getHttpData-Error: get response failed: response status code is "
        << res.value->status << "and response message is " << res.message;
    return LCNE_FAIL;
  }
  LcneResult generateNotifyReqBody(std::string &req_body)
  {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root = doc.NewElement("create-subscription");
    if (!root)
    {
      LOG_ERROR << "generateNotifyReqBody-Error: create create-subscription "
                   "element failed";
      return LCNE_FAIL;
    }
    root->SetAttribute("xmlns",
                       "urn:ietf:params:xml:ns:netconf:notification:1.0");
    doc.InsertFirstChild(root);
    tinyxml2::XMLElement *ip = doc.NewElement("ip");
    if (!ip)
    {
      LOG_ERROR << "generateNotifyReqBody-Error: create ip element failed";
      return LCNE_FAIL;
    }
    ip->SetText(LCNE_LOCAL_IP);
    root->InsertEndChild(ip);
    tinyxml2::XMLElement *port = doc.NewElement("port");
    if (!port)
    {
      LOG_ERROR << "generateNotifyReqBody-Error: create port element failed";
      return LCNE_FAIL;
    }
    port->SetText(LCNE_PORT);
    root->InsertEndChild(port);
    tinyxml2::XMLElement *url = doc.NewElement("url");
    if (!url)
    {
      LOG_ERROR << "generateNotifyReqBody-Error: create url element failed";
      return LCNE_FAIL;
    }
    url->SetText(LCNE_NOTIFY_TOPO_PATH);
    root->InsertEndChild(url);

    tinyxml2::XMLPrinter printer(nullptr, /* compact = */ true);
    doc.Accept(&printer);
    req_body = printer.CStr();
    return LCNE_SUCCESS;
  }
  LcneResult postLinkInfoNotify()
  {

    rack::com::RackHttpClient client(std::string(LCNE_URL) + ":" +
                                     std::string(LCNE_PORT));
    rack::com::RackComContext ctx;
    ctx.metadata["Content-Type"] = LCNE_CONTENT_TYPE;
    ctx.metadata["Accept"] = LCNE_CONTENT_TYPE;
    rack::com::RackHttpRequest req;
    req.method = rack::com::RackHttpMethod::POST;
    req.path = std::string(LCNE_NOTIFY_LINK_REQ_PATH);
    std::string reqBodyStr;
    if (generateNotifyReqBody(reqBodyStr) == LCNE_FAIL)
    {
      LOG_ERROR
          << "postLinkInfoNotify-Error: generate notify request body failed";
      return LCNE_FAIL;
    }
    LOG_INFO << "postLinkInfoNotify-Info: send request to "
             << std::string(LCNE_URL) + ":" + std::string(LCNE_PORT) + req.path;
    auto res = client.Do(ctx, req);
    if (res.Ok())
    {
      const auto &resp = res.value;
      if (resp->status == rack::com::Created_201)
      {
        LOG_INFO << "postLinkInfoNotify-Info: notify link info success";
        return LCNE_SUCCESS;
      }
      if (resp->status == rack::com::PreconditionFailed_412)
      {
        LOG_ERROR << "postLinkInfoNotify-Error: notify link info repeated";
        return LCNE_SUCCESS;
      }
      LOG_ERROR << "postLinkInfoNotify-Error: notify link info failed: response "
                   "status code is "
                << resp->status << "and response message is " << resp->body;
      return LCNE_FAIL;
    }
    LOG_ERROR << "postLinkInfoNotify-Error: post link info notify failed: "
              << res.message;
    return LCNE_FAIL;
  }

  bool mkdirRecursive(const std::string &dir)
  {
    struct stat info;
    if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
    {
      return true;
    }
    size_t pos = dir.find_last_of('/');
    if (pos != std::string::npos && pos > 0)
    {
      if (!mkdirRecursive(dir.substr(0, pos)))
      {
        LOG_ERROR << "mkdirRecursive-Error: create directory " << dir.substr(0, pos) << " failed";
        return false;
      }
    }
    return mkdir(dir.c_str(), lcne::common::CONFIG_PATH_PERM_755) == 0 || errno == EEXIST;
  }

  LcneResult SaveIpToConfigFile(std::string master_ip)
  {
    tinyxml2::XMLDocument doc;
    string filePath = std::string(lcne::common::CONFIG_PATH) + "/" + std::string(lcne::common::CONFIG_FILE);
    bool fileExist = std::ifstream(filePath).good();
    if (fileExist)
    {
      tinyxml2::XMLError result = doc.LoadFile(filePath.c_str());
      if (result != tinyxml2::XML_SUCCESS)
      {
        LOG_ERROR << "SaveIpToConfigFile-Error: load config file " << filePath << " failed";
        return LCNE_FAIL;
      }
      LOG_INFO << "SaveIpToConfigFile-Info: config file " << filePath << " loaded";
    }
    else
    {
      if (!mkdirRecursive(lcne::common::CONFIG_PATH))
      {
        LOG_ERROR << "SaveIpToConfigFile-Error: create directory " << lcne::common::CONFIG_PATH << " failed";
        return LCNE_FAIL;
      }

      doc.InsertFirstChild(doc.NewDeclaration());
      doc.InsertEndChild(doc.NewElement(""));
    }
    tinyxml2::XMLElement *root = doc.RootElement();
    if (!root)
    {
      LOG_ERROR << "SaveIpToConfigFile-Error: create root element failed";
      return LCNE_FAIL;
    }
    tinyxml2::XMLElement *ipElem = root->FirstChildElement("ip");
    if (ipElem)
    {
      ipElem->SetText(master_ip.c_str());
      LOG_DEBUG << "SaveIpToConfigFile-Debug: set ip element text to " << master_ip;
    }
    else
    {
      ipElem = doc.NewElement("ip");
      ipElem->SetText(master_ip.c_str());
      root->InsertEndChild(ipElem);
      LOG_DEBUG << "SaveIpToConfigFile-Debug: create ip element and set text to " << master_ip;
    }
    tinyxml2::XMLError saveResult = doc.SaveFile(filePath.c_str(), true);
    if (saveResult != tinyxml2::XML_SUCCESS)
    {
      LOG_ERROR << "SaveIpToConfigFile-Error: save config file " << filePath << " failed";
      return LCNE_FAIL;
    }
    LOG_INFO << "SaveIpToConfigFile-Info: config file " << filePath << " saved";
    return LCNE_SUCCESS;
  }
} // namespace lcne::common