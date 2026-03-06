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

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "common/rack_com_context.h"
#include "common/rack_com_result.h"
#include "http/rack_http_types.h"

namespace rack::com {
class RackHttpServerHandler {
public:
    static RackHttpServerHandler &GetInstance();
    RackHttpServerHandler(const RackHttpServerHandler &) = delete;
    RackHttpServerHandler &operator=(const RackHttpServerHandler &) = delete;

    void Register(RackHttpMethod method, std::string pathPattern, RackHttpHandler handler);

    void Use(RackHttpMiddleware middleware);

    RackComResult<RackHttpResponse> Dispatch(const RackComContext &context, const RackHttpRequest &request) const;
    const auto &routes() const { return routes_; }
    const auto &middlewares() const { return middlewares_; }

private:
    RackHttpServerHandler() = default;
    std::unordered_map<std::string, RackHttpHandler> routes_;
    std::vector<RackHttpMiddleware> middlewares_;
    std::mutex mutex_;
    static std::string MakeKey(RackHttpMethod method, std::string path);
};
} // namespace rack::com