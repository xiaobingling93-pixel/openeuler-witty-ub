#define MODULE_NAME "HTTP"
#include "http/rack_http_server.h"

#include <httplib.h>

#include "http/rack_http_server_handler.h"
#include "logger.h"

namespace rack::com {
std::unique_ptr<RackHttpServer> RackHttpServer::instance_ = nullptr;
std::once_flag RackHttpServer::init_flag_;
constexpr uint32_t START_TIMEOUT = 3000;
constexpr uint32_t START_SLEEP_TIME = 100;
constexpr uint32_t STOP_TIMES = 500;
constexpr uint32_t STOP_SLEEP_TIME = 5;

RackHttpServer &RackHttpServer::GetInstance()
{
    LOG_INFO << "Start GetInstance";
    if (!instance_) {
        LOG_ERROR << "RackHttpServer not initialized! Call Initialize() first.";
    }
    return *instance_;
}

bool RackHttpServer::Initialize(int port)
{
    std::call_once(init_flag_, [port]() {
        instance_.reset(new RackHttpServer(port));
        LOG_INFO << "RackHttpServer initialized";
    });
    return instance_ != nullptr;
}

bool RackHttpServer::Start()
{
    LOG_INFO << "Start()";
    std::lock_guard<std::mutex> lock(start_mutex_);
    LOG_INFO << "Start Check is running";
    if (server_.is_running()){
        LOG_INFO << "Server is running";
        return true;
    }
    LOG_INFO << "Start Run";
    try {
        thread_ = std::thread(&RackHttpServer::Run, this);
    } catch (const std::system_error &) {
        LOG_ERROR << "Failed to create thread.";
        return false;
    }
    LOG_INFO << "Start Retry";
    uint32_t retryTime = 0;
    while (retryTime <= START_TIMEOUT) {
        if (server_.is_running()) {
            return true;
        }
        retryTime += START_SLEEP_TIME;
        std::this_thread::sleep_for(std::chrono::milliseconds(START_SLEEP_TIME));
    }
    return server_.is_running();
}

void RackHttpServer::Stop()
{
    if (!server_.is_running()) return;
    server_.stop();
    bool exited = false;
    for (int i = 0; i < STOP_TIMES; ++i) {
        if (!server_.is_running()) {
            exited = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(STOP_SLEEP_TIME));
    }

    if (!exited) {
        LOG_WARN << "Server did not stop gracefully!";
        // Todo:terminate the process forcibly.
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

void RackHttpServer::Run()
{
    LOG_INFO << "In run";
    try {
        server_.new_task_queue = []() -> httplib::ThreadPool * { return new httplib::ThreadPool(4, 4); };

        server_.Get(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        server_.Post(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        server_.Put(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        server_.Delete(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        server_.Patch(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        server_.Options(".*",
            [this](const httplib::Request &request, httplib::Response &response) { HandlerRequest(request, response); });
        
        // 检查端口是否被占用
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket.");
        }

        int reuse = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        //尝试绑定
        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Port " + std::to_string(port_) + " is already in use.");
        }

        // 关闭套接字
        close(sockfd);
        std::string eid = "127.0.0.1";
        server_.listen(eid, port_);

    } catch (const std::exception &e) {
        LOG_ERROR << "Error starting server: " << e.what();
    }
}

static void ProcessRequestHeadersAndParams(const httplib::Request &req, RackHttpRequest &request)
{
    for (const auto &pair : req.headers) {
        if (request.headers.find(pair.first) == request.headers.end()) {
            request.headers[pair.first] = pair.second;
        }
    }

    for (const auto &pair : req.params) {
        if (request.queryParams.find(pair.first) == request.queryParams.end()) {
            request.queryParams[pair.first] = pair.second;
        }
    }
}

RackResult RackHttpServer::ValidateHttpRequest(const httplib::Request &req, RackHttpRequest &request)
{
    if (!req.params.empty()) {
        std::string queryStr = GenerateQueryString(req.params);
        if (queryStr.size() > httpMaxQuerySize) {
            LOG_ERROR << "HttpMsg queryParams is oversize";
            return RACK_FAIL;
        }
    }
    if (!req.body.empty() && req.body.size() > httpMaxBodySize) {
        LOG_ERROR << "HttpRequest BodyLen is oversize";
        return RACK_FAIL;
    }
    request.method = StringToMethod(req.method);
    if (request.method == RackHttpMethod::INVALID) {
        LOG_ERROR << "The request method is not support";
        return RACK_FAIL;
    }
    return RACK_OK;
}
std::string RackHttpServer::GenerateQueryString(const std::multimap<std::string, std::string> &queryParams)
{
    std::string queryString;
    bool first = true;
    for (const auto &param : queryParams) {
        if (!first) {
            queryString.append("&"); // 添加分隔符
        }
        first = false;
        queryString.append(param.first);
        queryString.append("=");
        queryString.append("param.second");
    }
    return queryString;
}

void RackHttpServer::HandlerRequest(const httplib::Request &req, httplib::Response &res)
{
    LOG_INFO << "Receive request, url: " << req.path << "method: " << req.method;
    RackComContext ctx;
    ctx.cancelled = nullptr;
    ctx.deadline = std::chrono::steady_clock::time_point::max();
    for (const auto &[k, v] : req.headers) {
        ctx.metadata.emplace(k, v);
    }

    RackHttpRequest request{};
    //Set request method in this function
    if (ValidateHttpRequest(req, request) != RACK_OK) {
        res.status = BadRequest_400;
        res.set_content("The request is invalid.", "text/plain");
        return;
    }
    request.remote_addr = req.remote_addr;
    request.remote_port = req.remote_port;
    request.path = req.path;
    request.body = req.body;
    ProcessRequestHeadersAndParams(req, request);
    RackComResult<RackHttpResponse> result = RackHttpServerHandler::GetInstance().Dispatch(ctx, request);
    if (!result.Ok()) {
        res.status = 500;
        res.set_content(result.message, "text/plain");
        return;
    }

    const RackHttpResponse &resp = std::move(*result.value);
    res.status = resp.status;

    for (const auto &header : resp.headers) {
        res.set_header(header.first, header.second);
    }

    const std::string &content_type = resp.headers.count("Content-Type") ? resp.headers.at("Content-Type") :
                                                                           "text/plain";
    res.set_content(resp.body, content_type);
}

} // namespace rack::com