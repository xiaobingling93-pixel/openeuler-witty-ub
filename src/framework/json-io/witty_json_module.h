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

#ifndef WITTY_JSON_MODULE_H
#define WITTY_JSON_MODULE_H
#include "rack_error.h"
#include "rack_module.h"
#include "witty_json.h"
namespace witty_json::module {
using namespace rack::module;
using namespace witty_json::io;
class JSONModule : public RackModule {
public:
  ~JSONModule() override = default;
  RackResult Initialize() override;
  void UnInitialize() override;
  RackResult Start() override;
  void Stop() override;
  JSONModule() = default;

  template <typename... Pairs>
  RackResult WriteVectorsToFile(const std::string &filename, Pairs &&...pairs) {
    auto ret =
        json_io->WriteVectorsToFile(filename, std::forward<Pairs>(pairs)...);
    if (ret != RACK_OK) {
      LOG_ERROR
          << "JsonModule::WriteVectorsTofile-Error: failed to write to file "
          << filename;
    }
    return ret;
  }
  template<typename T>
  auto GetJsonPair(const char *key, const std::vector<T> &vec){
    return std::make_pair(key, std::ref(vec));
  }

private:
  std::shared_ptr<WittyJson> json_io;
};

} // namespace witty_json::module
#endif // WITTY_JSON_MODULE_H
