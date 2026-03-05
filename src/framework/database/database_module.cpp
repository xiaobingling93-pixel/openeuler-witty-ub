#include <memory>
#include <iostream>
#include "database_module.h"
#include "rack_error.h"

namespace database {
using namespace rack::module;

RackResult DatabaseModule::Initialize()
{
    db = make_shared<Database>();
    std::cout << "DatabaseModule Initialized" << std::endl;
    return RACK_OK;
}
void DatabaseModule::Unitialize()
{
    return;
}
RackResult DatabaseModule::Start()
{
    OP_RET rc = db -> OpenDb("euler_copilot_ub", true);
    if (rc == OP_RET::FAIL) {
        std::cout << "DatabaseModule Started Fail" << std::endl;
        return RACK_FAIL;
    } else {
        std::cout << "DatabaseModule Started" << std::endl;
        return RACK_OK;
    }
}
void DatabaseModule::Stop()
{
    //OP_RET rc = db -> CloseDb();
    return;
}
shared_ptr<Database> DatabaseModule::GetDatabase()
{
    cout << "Get Database" << endl;
    return db;
}
}