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

#define MODULE_NAME "HTTP_MODULE"
#include "rack_http_module.h"
#include "rack_http_server.h"
#include "logger.h"

namespace rack::com {
RackResult HttpModule::Initialize()
{
    return RACK_OK;
}

void HttpModule::UnInitialize() {}

RackResult HttpModule::Start()
{
    if (!RackHttpServer::GetInstance().Start()) {
        LOG_ERROR << "http tcp server start failed!";
        return RACK_FAIL;
    }
    return RACK_OK;
}

void HttpModule::Stop()
{
    RackHttpServer::GetInstance().Stop();
    LOG_INFO << "http stop end";
}

}  // namespace rack::com