#ifndef UBSE_CONTEXT_H
#define UBSE_CONTEXT_H
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <string>
#include <memory>
#include <iostream>
#include <queue>
#include <map>
#include "rack_module.h"
#include "rack_error.h"
#include "logger.h"

namespace ubse::context {
using namespace rack::module;
struct TopoToolsArgs{
    std::string network_mode;
    std::string pod_mode;
    // When pod_mode is on, key is pod_id, value is log path.
    // When pod_mode is off, key is normal, value is log path, default is /var/log/message
    std::map<std::string, std::string> umq_log_path_map;
    std::vector<std::string> pod_id_list;
};
class UbseContext{
    public: 
        static UbseContext &GetInstance(){
            static UbseContext instance;
            return instance;
        }
        template<typename T>
        void AddModuleDependencies(){
            T module;
            moduleDeps[typeid(T)] = module.GetDependencies();
            for(std::type_index dep : module.GetDependencies()){
                LOG_INFO << "UbseContext::AddModuleDependencies-Info: module " << typeid(T).name() << " depends on " << dep.name();
            }
            return;
        }
        RackResult ParseArgs(int argc, char* argv[]);
        template<typename T>
        RackResult RegisterModule(){
            //静态断言:确保T完整且是是RackModule的派生类
            LOG_INFO << "UbseContext::RegisterModule-Info: module " << typeid(T).name();
            static_assert(sizeof(T) != 0, "Type is incomplete. Provide a full definition.");
            if (moduleMap.find(typeid(T)) == moduleMap.end()){
                LOG_ERROR << "UbseContext::RegisterModule-Error: module " << typeid(T).name() << " not defined";
                return RACK_FAIL;
            }
            std::queue<std::type_index> moduleQueue;
            moduleQueue.push(typeid(T));
            while(!moduleQueue.empty()){
                std::type_index currentModule = moduleQueue.front();
                moduleQueue.pop();
                if(initModules.find(currentModule) == initModules.end()){
                    initModules.insert(currentModule);
                    for(auto dep : moduleDeps[currentModule]){
                        moduleQueue.push(dep);
                        LOG_INFO<< "UbseContext::RegisterModule-Info: module " << currentModule.name() << " depends on " << dep.name();
                        }
                    }
                }
                return RACK_OK;
            }
        template<typename T>
        RackResult InitModule(std::shared_ptr<RackModule> module){
            moduleMap[typeid(T)] = module;
            return RACK_OK;
        }
        std::vector<std::type_index> GetSortedModules();
        std::unordered_map<std::type_index, std::shared_ptr<RackModule>> GetModuleMap();
        std::string GetRole();
        RackResult InitAndStartModules();
        const std::unordered_map<std::string, std::string>& GetArgMap() const;
        RackResult Run(int argc, char* argv[]);
        template<typename T>
        std::shared_ptr<T> GetModule(){
            static_assert(sizeof(T) != 0, "Type is incomplete. Provide a full definition.");
            static_assert(std::is_base_of<RackModule, T>::value, "GetModule must be used with RackModule derived types");
            const std::type_index moduleType = typeid(T);
            try{
                auto it = moduleMap.find(moduleType);
                if(it!=moduleMap.end()){
                    return std::dynamic_pointer_cast<T>(it->second);
                }
            }catch(...){
                return nullptr;
            }
            return nullptr;
        }
        RackResult ParseTopoToolsArgs(int argc, char* argv[]);
        TopoToolsArgs GetTopoToolsArgs(){return topoArgs;}
        
    private:
        RackResult SortModules();
        std::unordered_map<std::type_index, std::shared_ptr<RackModule>> moduleMap;
        std::unordered_map<std::type_index, std::vector<std::type_index>> moduleDeps;
        std::unordered_set<std::type_index> initModules;
        std::unordered_set<std::type_index> sortedModules;
        std::unordered_map<std::string, std::string> argMap;
        std::atomic_bool running_{true};
        ubse::context::TopoToolsArgs topoArgs;
};
} // namespace ubse::context
#endif // UBSE_CONTEXT_H