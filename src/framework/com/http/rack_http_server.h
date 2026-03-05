#pragma once

#include <httplib.h>

#include "http/rack_http_server_handler.h"
#include "rack_error.h"

namespace rack::com {
class RackHttpServer {
public:
    static RackHttpServer &GetInstance();
    static bool Initialize(int port=8080);
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