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

#include <functional>
#include <string>
#include <map>

#include "common/rack_com_context.h"
#include "common/rack_com_result.h"

namespace rack::com {

constexpr size_t httpMaxQuerySize = 11264;
constexpr size_t httpMaxBodySize = 524288;
enum StatusCode {
    OK_200 = 200,
    Created_201 = 201,
    BadRequest_400 = 400,
    PreconditionFailed_412 = 412,
    NotFound_404 = 404,
    InternalServerError_500 = 500,
};
enum class RackHttpMethod { GET, POST, PUT, DELETE_, PATCH, OPTIONS, INVALID };

static std::string MethodToString(RackHttpMethod method)
{
    switch (method) {
        case RackHttpMethod::GET:
            return "GET";
        case RackHttpMethod::POST:
            return "POST";
        case RackHttpMethod::PUT:
            return "PUT";
        case RackHttpMethod::DELETE_:
            return "DELETE";
        case RackHttpMethod::PATCH:
            return "PATCH";
        case RackHttpMethod::OPTIONS:
            return "OPTIONS";
    }
    return "INVALID";
}

static RackHttpMethod StringToMethod(const std::string &method)
{
    if (method == "GET")
        return RackHttpMethod::GET;
    if (method == "POST")
        return RackHttpMethod::POST;
    if (method == "PUT")
        return RackHttpMethod::PUT;
    if (method == "DELETE")
        return RackHttpMethod::DELETE_;
    if (method == "PATCH")
        return RackHttpMethod::PATCH;
    if (method == "OPTIONS")
        return RackHttpMethod::OPTIONS;

    return RackHttpMethod::INVALID;
}

struct RackHttpRequest {
    RackHttpMethod method = RackHttpMethod::GET;
    std::string path = "/";
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;
    std::string remote_addr;
    int remote_port = -1;
};

struct RackHttpResponse {
    int status = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

using RackHttpHandler = 
    std::function<RackComResult<RackHttpResponse>(const RackComContext &context, const RackHttpRequest &request)>;

using RackHttpMiddleware = std::function<RackHttpHandler(RackHttpHandler handler)>;
} // namespace rack::com