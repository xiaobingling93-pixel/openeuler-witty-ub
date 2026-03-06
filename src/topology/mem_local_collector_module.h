#ifndef MEM_LOCAL_COLLECTOR_MODULE_H
#define MEM_LOCAL_COLLECTOR_MODULE_H
#include <string>
#include <vector>
#include <memory>
#include "rack_error.h"
#include "rack_module.h"
#include "database_module.h"
#include "mem_collector.h"

namespace topology::mem {
using namespace rack::module;
using namespace database;
class MemLocalCollectorModule : public RackModule {
public:
    MemLocalCollectorModule()
    {
        dependencies.push_back(typeid(DatabaseModule));
    }
    ~MemLocalCollectorModule() override = default;
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