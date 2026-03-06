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

#define MODULE_NAME "WITTY_JSON"
#include "witty_json_module.h"
#include "logger.h"
#include <iostream>
#include <memory>

namespace witty_json::module
{
  using namespace rack::module;
  RackResult JSONModule::Initialize()
  {
    json_io = std::make_shared<WittyJson>();
    LOG_INFO << "JsonModule::Initialize-Info: module initialized";
    return RACK_OK;
  }
  void JSONModule::UnInitialize() { return; }
  RackResult JSONModule::Start() { return RACK_OK; }
  void JSONModule::Stop() { return; }
} // namespace witty_json::module