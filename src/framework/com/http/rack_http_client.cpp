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

#include "http/rack_http_client.h"
#include "logger.h"
#include <httplib.h>
#include <sstream>

namespace rack::com {
RackHttpClient::RackHttpClient(std::string baseUrl) : baseUrl_(baseUrl)
{
}

RackComResult<RackHttpResponse> RackHttpClient::Do(const RackComContext &context, const RackHttpRequest &request)
{
    httplib::Client cli(baseUrl_);

    httplib::Headers headers;
    for (const auto &[k, v] : request.headers) {
        headers.emplace(k, v);
    }

    httplib::Result res;
    switch (request.method) {
        case RackHttpMethod::GET:
            res = cli.Get(request.path, headers);
            break;
        case RackHttpMethod::POST: {
            std::string content_type = "text/plain";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                content_type = it->second;
                headers.erase("Content-Type");
            }

            res = cli.Post(request.path, headers, request.body, content_type);
            break;
        }
        case RackHttpMethod::PUT: {
            std::string content_type = "text/plain";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                content_type = it->second;
                headers.erase("Content-Type");
            }

            res = cli.Put(request.path, headers, request.body, content_type);
            break;
        }
        case RackHttpMethod::DELETE_:
            res = cli.Delete(request.path, headers);
            break;
        case RackHttpMethod::OPTIONS:
            res = cli.Options(request.path);
            break;
        default:
            return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, "http method not supported");
    }

    httplib::Response resp;
    if (!res) {
        std::ostringstream oss;
        oss << "http request failed"
            << ", method: " << MethodToString(request.method)
            << ", url: " << baseUrl_ << request.path
            << ", error: " << httplib::to_string(res.error());
        LOG_ERROR << "RackHttpClient::Do-Error: " << oss.str();
        return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, oss.str());
    }
    resp = res.value();

    RackHttpResponse out;
    out.status = resp.status;
    out.body = resp.body;

    for (const auto &[k, v] : resp.headers) {
        out.headers[k] = v;
    }

    return RackComResult<RackHttpResponse>::Ok(std::move(out));
}
} // namespace rack::com
