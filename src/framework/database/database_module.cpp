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
void DatabaseModule::UnInitialize()
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