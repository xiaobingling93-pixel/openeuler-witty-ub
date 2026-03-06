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

#include <httplib.h>

#include "http/rack_http_server_handler.h"
#include "rack_error.h"

namespace rack::com {
class RackHttpServer {
public:
    static RackHttpServer &GetInstance();
    static bool Initialize(int port = 8080);
    RackHttpServer(const RackHttpServer &) = delete;
    RackHttpServer &operator=(const RackHttpServer &) = delete;
    ~RackHttpServer(){
        Stop();
    }
    void HandlerRequest(const httplib::Request &req, httplib::Response &res);
    RackResult ValidateHttpRequest(const httplib::Request &req, RackHttpRequest &request);
    std::string GenerateQueryString(const std::multimap<std::string, std::string> &queryParams);
    bool Start();
    void Stop();
    void Run();

private:
    RackHttpServer(int port):port_(port){}

    int port_;
    httplib::Server server_;
    std::thread thread_;
    bool running_ = false;
    std::mutex start_mutex_;

    static std::unique_ptr<RackHttpServer> instance_;
    static std::once_flag init_flag_;
};
} // namespace rack::com