#ifndef MEM_COLLECTOR_H
#define MEM_COLLECTOR_H
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "mem_data_def.h"
#include "rack_module.h"
#include "ubse_context.h"
#include "database.h"

namespace topology::mem {
using namespace ubse::context;
using namespace database;
class MemCollector {
public:
    OP_RET InitDb(shared_ptr<Database> db_);
    OP_RET StartDb();
    shared_ptr<Database> GetDb();
    OP_RET InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMems);
    OP_RET InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMems);
private:
    unordered_map<uint32_t, vector<ExportMemory>> exportMemoryInfos;
    unordered_map<uint32_t, vector<ExportMemory>> importMemoryInfos;
    shared_ptr<Database> db;
};
}
#endif // MEM_COLLECTOR_H