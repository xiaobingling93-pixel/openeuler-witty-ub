#include <memory>
#include <vector>
#include <tuple>
#include <string>
#include "mem_collector.h"
#include "database.h"

namespace topology::mem {
using namespace database;
OP_RET MemCollector::InitDb(std::shared_ptr<Database> db_)
{
    db = db_;
    std::cout << "init db" << std::endl;
    return SUCCESS;
}
OP_RET MemCollector::StartDb()
{
    OP_RET ret = db -> CreateTable("ExportMemory", vector<tuple<string, string, bool, bool>>({
        make_tuple("tokenId", "TEXT", true, false),
        make_tuple("cachable", "TEXT", true, false),
        make_tuple("ubaddr", "TEXT", true, true),
        make_tuple("size", "TEXT", true, false),
        make_tuple("deviceId", "TEXT", true, false),
        make_tuple("memId", "TEXT", true, false),
        make_tuple("isMemoryFromUser", "TEXT", true, false)}));
    if (ret != SUCCESS) {
        return ret;
    }
    ret = db -> CreateTable("ImportMemory", vector<tuple<string, string, bool, bool>>({
        make_tuple("scna", "TEXT", true, false),
        make_tuple("dcna", "TEXT", true, false),
        make_tuple("tokenId", "TEXT", true, false),
        make_tuple("cachable", "TEXT", true, false),
        make_tuple("ubaddr", "TEXT", true, true),
        make_tuple("size", "TEXT", true, false),
        make_tuple("deviceId", "TEXT", true, false),
        make_tuple("isNumaImport", "TEXT", true, false),
        make_tuple("numaId", "TEXT", true, false),
        make_tuple("pid", "TEXT", true, false),
        make_tuple("gid", "TEXT", true, false),
        make_tuple("uid", "TEXT", true, false),
        make_tuple("memId", "TEXT", true, false),
        make_tuple("isDecoderPrefilled", "TEXT", true, false),
        make_tuple("paddr", "TEXT", true, false)}));
    if (ret != SUCCESS) {
        return ret;
    }
    return SUCCESS;
}
shared_ptr<Database> MemCollector::GetDb()
{
    return db;
}
OP_RET MemCollector::InsertImportMemoryData(vector<unordered_map<std::string, std::string>> &importMemories)
{
    OP_RET res = OP_RET::SUCCESS;
    for (auto importMemory: importMemories) {
        OP_RET ret = db -> InsertData("ImportMemory", importMemory);
        if (ret != OP_RET::SUCCESS) {
            res = ret;
        }
    }
    return res;
}
OP_RET MemCollector::InsertExportMemoryData(vector<unordered_map<std::string, std::string>> &exportMemories)
{
    OP_RET res = OP_RET::SUCCESS;
    for (auto exportMemory: exportMemories) {
        OP_RET ret = db -> InsertData("ExportMemory", exportMemory);
        if (ret != OP_RET::SUCCESS) {
            res = ret;
        }
    }
    return res;
}
}