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

#define MODULE_NAME "COM-HTTP"

#include "http/rack_http_client.h"
#include <httplib.h>
#include "logger.h"

template <typename Sender>
httplib::Result SendWithBody(httplib::Headers &headers, Sender &&sender)
{
    std::string content_type = "text/plain";
    auto it = headers.find("Content-Type");
    if (it != headers.end()) {
        content_type = it->second;
        headers.erase("Content-Type");
    }
    return sender(content_type);
}

namespace rack::com {
RackHttpClient::RackHttpClient(std::string baseUrl) : baseUrl_(baseUrl) {}

RackComResult<RackHttpResponse> RackHttpClient::Do(const RackComContext &context, const RackHttpRequest &request)
{
    httplib::Client cli(baseUrl_);
    httplib::Headers headers;
    for (const auto &[k, v] : request.headers)
        headers.emplace(k, v);
    httplib::Result res;
    switch (request.method) {
        case RackHttpMethod::GET:
            res = cli.Get(request.path, headers);
            break;
        case RackHttpMethod::POST:
            res = SendWithBody(headers, [&cli, &request, &headers](const std::string &content_type) {
                return cli.Post(request.path, headers, request.body, content_type);
            });
            break;
        case RackHttpMethod::PUT:
            res = SendWithBody(headers, [&cli, &request, &headers](const std::string &content_type) {
                return cli.Put(request.path, headers, request.body, content_type);
            });
            break;
        case RackHttpMethod::DELETE_:
            res = cli.Delete(request.path, headers);
            break;
        case RackHttpMethod::OPTIONS:
            res = cli.Options(request.path);
            break;
        default:
            return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, "http method not supported");
    }
    if (!res) {
        std::string msg = "http request failed: " + httplib::to_string(res.error());
        LOG_ERROR << "RackHttpClient::Do-Error: " << msg;
        return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, msg);
    }
    httplib::Response resp = res.value();
    RackHttpResponse out;
    out.status = resp.status;
    out.body = resp.body;
    for (const auto &[k, v] : resp.headers)
        out.headers[k] = v;
    return RackComResult<RackHttpResponse>::Ok(std::move(out));
}
} // namespace rack::com
