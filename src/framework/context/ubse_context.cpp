#define MODULE_NAME "CONTEXT"
#include <string>
#include <iostream>
#include <algorithm>
#include <typeindex>
#include <memory>
#include <thread>
#include <map>
#include <set>
#include "rack_error.h"
#include "rack_module.h"
#include "ubse_context.h"
#include "logger.h"

namespace ubse::context {
    RackResult UbseContext::ParseArgs(int argc, char* argv[]){
        for(int i = 1; i < argc; i++){
            std::string arg = argv[i];
            if(arg.substr(0, 2) != "--"){
                LOG_ERROR << "UbseContext::ParseArgs-Error: parsing arguments " << arg;
                return RACK_FAIL;
            }
            std::string key = arg.substr(2);
            if (key.empty()){
                LOG_ERROR << "UbseContext::ParseArgs-Error: parsing arguments " << arg;
                return RACK_FAIL;
            }
            std::string value;
            if (i+1 < argc && argv[i+1][0] != '-'){
                value = argv[i+1];
            }
            argMap[key] = value;
        }
        LOG_INFO << "UbseContext::ParseArgs-Args ";
        for(const auto &it : argMap){
            LOG_INFO << "[" << it.first << ":" << it.second << "]";
        }
        return RACK_OK;
    }
    RackResult UbseContext::SortModules(){
        sortedModules.clear();
        std::queue<std::type_index> moduleQueue;
        for(auto module: initModules){
            moduleQueue.push(module);
        }
        int cnt=0;
        while(!moduleQueue.empty()){
            if (cnt == moduleQueue.size()){
                LOG_ERROR << "UbseContext::SortModules-Error: circular dependency detected";
                return RACK_FAIL;
            }
            std::type_index currentModule = moduleQueue.front();
            moduleQueue.pop();
            bool flag = true;
            for(auto dep : moduleDeps[currentModule]){
                if(find(sortedModules.begin(), sortedModules.end(), dep) == sortedModules.end()){
                    flag = false;
                    break;
                }
            }
            if(flag){
                sortedModules.push_back(currentModule);
                cnt=0;
            }else{
                moduleQueue.push(currentModule);
                cnt+=1;
            }
        }
        return RACK_OK;
    }
    std::string UbseContext::GetRole(){
        if(argMap.find("role") == argMap.end()){
            LOG_ERROR << "UbseContext::GetRole-Error: agr role not defined";
            return "";
        }
        std::string role = argMap["role"];
        LOG_INFO << "UbseContext::GetRole-Info: role is " << role;
        if(role == "analyzer"){
            return role;
        }else if(role == "collector"){
            return role;
        }else{
            LOG_ERROR << "UbseContext::GetRole-Error: role " << role << " not supported";
            return "UNKNOWN";
        }
    }
    std::vector<std::type_index> UbseContext::GetSortedModules(){
        SortModules();
        return sortedModules;
    }
    std::unordered_map<std::type_index, std::shared_ptr<RackModule>> UbseContext::GetModuleMap(){
        return moduleMap;
    }
    RackResult UbseContext::InitAndStartModules(){
        RackResult ret = RackResult::RACK_OK;
        for(std::type_index moduleType : sortedModules){
            std::shared_ptr<RackModule> module = moduleMap[moduleType];
            ret = module->Initialize();
            if(ret != RACK_OK){
                LOG_ERROR << "UbseContext::InitAndStartModules-Error: module " << moduleType.name() << " init failed";
                return ret;
            }
        }
        for(std::type_index moduleType : sortedModules){
            std::shared_ptr<RackModule> module = moduleMap[moduleType];
            ret = module->Start();
            if(ret != RACK_OK){
                LOG_ERROR << "UbseContext::InitAndStartModules-Error: module " << moduleType.name() << " start failed";
                return ret;
            }
        }
        return ret;
    }
    const std::unordered_map<std::string, std::string>& UbseContext::GetArgMap() const{
        return argMap;
    }
    RackResult UbseContext::Run(int argc, char* argv[]){
        RackResult ret = InitAndStartModules();
        if(ret != RACK_OK){
            LOG_ERROR << "UbseContext::Run-Error: init and start modules failed";
            return ret;
        }
        return ret;
    }
    std::vector<std::string> split(const std::string& s, char delimiter){
        std::vector<std::string> tokens;
        size_t start = 0, end = 0;
        while((end = s.find(delimiter, start)) != std::string::npos){
            tokens.push_back(s.substr(start, end - start));
            start = end + 1;
        }
        if(start < s.size()){
            tokens.push_back(s.substr(start));
        }
        return tokens;
    }
    bool isValidPodId(const std::string &id){
        if(id.empty()){
            return false;
        }
        if(id.size() > 253){
            return false;
        }

        if (!std::isalnum(static_cast<unsigned char>(id[0]))){
            return false;
        }
        if(!std::isalnum(static_cast<unsigned char>(id.back()))){
            return false;
        }
        for(char c:id ){
            if(!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='-')){
                return false;
            }
        }
        return true;
    }
    bool isValidPathEntry(const std::string &entry, std::string &outPodId, std::string &outPath, std::string &outError){
        size_t pos = entry.find(':');
        if(pos == std::string::npos){
            outError = "missing or invalid pod id";
            return false;
        }
        outPodId = entry.substr(0, pos);
        outPath = entry.substr(pos + 1);
        if(!isValidPodId(outPodId)){
            outError = "invalid pod id characters";
            return false;
        }
        if (outPath.empty() || outPath[0] != '/'){
            outError = "path must be absolute (start with '/')";
            return false;
        }
        return true;
    }
    RackResult UbseConetext::ParseTopoToolsArgs(int argc, char* argv[]){
        std::string network_mode;
        std::string pod_mode;
        std::string umq_log_path;
        std::string pod_id;

        bool has_network_mode = false, has_pod_mode = false, has_umq_log_path = false, has_pod_id = false;
        std::map<std::string, std::string> path_map;
        std::vector<std::string> pod_id_list;
        for(int i = 1; i < argc; ++i){
            std::string arg = argv[i];
            if(arg == "--network-mode"){
                if(i+1 >= argc){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing network mode value";
                    return RACK_FAIL;
                }
                network_mode = argv[++i];
                if(network_mode != "fullmesh" && network_mode != "clos"){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: invalid network mode value " << network_mode;
                    return RACK_FAIL;
                }
                LOG_DEBUG << "UbseConetext::ParseTopoToolsArgs-Debug: network mode is " << network_mode;
                has_network_mode = true;
            }else if(arg == "--pod-mode"){
                if(i+1 >= argc){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing pod mode value";
                    return RACK_FAIL;
                }
                pod_mode = argv[++i];
                if(pod_mode != "on" && pod_mode != "off"){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: invalid pod mode value " << pod_mode;
                    return RACK_FAIL;
                }
                LOG_DEBUG << "UbseConetext::ParseTopoToolsArgs-Debug: pod mode is " << pod_mode;
                has_pod_mode = true;
            }else if(arg == "--umq-log-path"){
                if(i+1 >= argc){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing umq log path value";
                    return RACK_FAIL;
                }
                umq_log_path = argv[++i];
                LOG_DEBUG << "UbseConetext::ParseTopoToolsArgs-Debug: umq log path is " << umq_log_path;
                has_umq_log_path = true;
            }else if(arg == "--pod-id"){
                if(i+1 >= argc){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing pod id value";
                    return RACK_FAIL;
                }
                pod_id = argv[++i];
                LOG_DEBUG << "UbseConetext::ParseTopoToolsArgs-Debug: pod id is " << pod_id;
                has_pod_id = true;
            }else{
                LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: parsing topo tools arguments " << arg;
                return RACK_FAIL;
            }
            
        if(!has_network_mode){
            LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing network mode";
            return RACK_FAIL;
        }
        if(!has_pod_mode){
            LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: missing pod mode";
            return RACK_FAIL;
        }
        if (pod_mode == "on"){
            if(!has_umq_log_path){
                LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: umq log path is required when pod mode is on";
                return RACK_FAIL;
            }
            auto entries  = split(umq_log_path, ',');
            if(entries.empty()){
                LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: umq log path is empty";
                return RACK_FAIL;
            }
            for(const auto &entry : entries){
                std::string podId, pathPart,error;
                if(!isValidPathEntry(entry, podId, pathPart, error)){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: invalid umq log path entry " << entry << " - " << error;
                    return RACK_FAIL;
                }
                if(path_map.count(podId)){
                    LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: pod id " << podId << " is duplicated in umq log path";
                    return RACK_FAIL;
                }
                path_map[podId] = pathPart;
            }

            if(has_pod_id){
                pod_id_list = split(pod_id, ',');
                std::set<std::string> allowed(path_map.begin(), path_map.end());
                for(const auto &[podId, _] : path_map){
                    allowed.insert(podId);
                }
                for(const auto &d : pod_id_list){
                    if(d.empty()){
                        LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: empty pod id in pod id list";
                        return RACK_FAIL;
                    }
                    if(allowed.find(d) == allowed.end()){
                        LOG_ERROR << "UbseConetext::ParseTopoToolsArgs-Error: pod id " << d << " not found in umq log path";
                        return RACK_FAIL;
                    }
                }
            }
        }else{
            if(!has_umq_log_path){
                umq_log_path = "/var/log/message";
            }
            path_map["normal"] = umq_log_path;
            LOG_DEBUG << "UbseConetext::ParseTopoToolsArgs-Debug: normal log path is " << umq_log_path;
        }
    }
    topoToolsArgs.network_mode = network_mode;
    topoToolsArgs.pod_mode = pod_mode;
    topoToolsArgs.umq_log_path_map = path_map;
    topoToolsArgs.pod_id_list = pod_id_list;
    LOG_INFO << "UbseConetext::ParseTopoToolsArgs-Info: Validation passed, ready to run ";
    return RACK_OK;
}
}