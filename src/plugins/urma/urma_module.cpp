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

#define MODULE_NAME "URMA"
#include <unordered_map>

#include "rack_error.h"
#include "urma_module.h"
#include "logger.h"
#include "ubse_context.h"
#include "urma_error.h"
#include "urma_topology.h"

namespace urma::module {
using namespace ubse::context;
RackResult URMAModule::Initialize()
{
    urmaTopology = std::make_shared<urma::topo::URMATopology>();
    return RACK_OK;
}
RackResult URMAModule::Start()
{
    LOG_DEBUG << "Execute URMAModule Start";
    UbseContext& g_rackContext = UbseContext::GetInstance();
    auto args = g_rackContext.GetTopoToolsArgs();
    URMAResult ret = urmaTopology->CreateTopology(args);
    if(ret == URMA_FAIL){
        LOG_ERROR << "Execute URMAModule Start Error";
        return RACK_FAIL;
    }
    LOG_DEBUG << "Execute URMAModule Success";
    return RACK_OK;
}
void URMAModule::UnInitialize()
{
    return;
}
void URMAModule::Stop()
{
    return;
}
} // namespace urma::module