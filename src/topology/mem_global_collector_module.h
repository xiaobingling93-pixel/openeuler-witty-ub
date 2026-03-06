#ifndef MEM_COLLECTOR_MODULE_H
#define MEM_COLLECTOR_MODULE_H
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "rack_module.h"
#include "rack_error.h"
#include "mem_collector.h"
#include "database_module.h"

namespace topology::mem {
using namespace rack::module;
using namespace database;
class MemGlobalCollectorModule : public RackModule {
public:
    MemGlobalCollectorModule()
    {
        dependencies.push_back(typeid(DatabaseModule));
    }
    ~MemGlobalCollectorModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
    shared_ptr<MemCollector> GetCollector()
    {
        return collector;
    }
    RackResult InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMemories);
    RackResult InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMemories);
private:
    shared_ptr<MemCollector> collector;
};
}
#endif // MEM_GLOBAL_COLLECTOR_MODULE_H