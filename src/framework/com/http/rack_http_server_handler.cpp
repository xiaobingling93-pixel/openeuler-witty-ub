#define MODULE_NAME "HTTP_MODULE"
#include "http/rack_http_server.h"
#include "logger.h"
#include "rack_http_server.h"

namespace rack::com {

RackHttpServerHandler &RackHttpServerHandler::GetInstance()
{
    static RackHttpServerHandler inst; // Meyers 单例：线程安全（C++11+）
    return inst;
}

void RackHttpServerHandler::Register(RackHttpMethod method, std::string pathPattern, RackHttpHandler handler)
{
    // 现在还用不到middlewares
    // for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it) {

    //     handler = (*it)(std::move(handler));
    // }

    std::string routeKey = MakeKey(method, std::move(pathPattern));
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.find(routeKey);
    if (it != routes_.end()) {
        LOG_WARN << "url: " << pathPattern << " has already been registered in tcp server.";
        return;
    }
    routes_[routeKey] = std::move(handler);
    LOG_INFO << "register url: " << pathPattern << " for http tcp server.";
    return;
}

void RackHttpServerHandler::Use(RackHttpMiddleware middleware)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.push_back(std::move(middleware));
}

RackComResult<RackHttpResponse> RackHttpServerHandler::Dispatch(const RackComContext &context,
    const RackHttpRequest &request) const
{
    if (context.cancelled && context.cancelled->load(std::memory_order_relaxed)) {
        return RackComResult<RackHttpResponse>::Error(RackComError::CANCELLED, "cancelled");
    }
    auto it = routes_.find(MakeKey(request.method, request.path));
    if (it != routes_.end()) {
        return it->second(context, request);
    }

    RackHttpHandler const *best = nullptr;
    size_t bestPrefixLen = 0;

    
    for (const auto &[k, h] : routes_) {
        const auto pos = k.find(' ');
        if (pos == std::string::npos) {
            continue;
        }
        if (k.substr(0, pos) != MethodToString(request.method)) {
            continue;
        }

        const std::string pattern = k.substr(pos + 1);
        if (pattern.size() >= 2 && pattern[pattern.size() - 2] == '/' && pattern[pattern.size() - 1] == '*') {
            const std::string prefix = pattern.substr(0, pattern.size() - 1);
            if (request.path.rfind(prefix, 0) == 0) {
                if (prefix.size() > bestPrefixLen) {
                    bestPrefixLen = prefix.size();
                    best = &h;
                }
            }
        }
    }

    if (best) {
        return (*best)(context, request);
    }

    return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, "route not found");
}

std::string RackHttpServerHandler::MakeKey(RackHttpMethod method, std::string path)
{
    if (path.empty()) {
        path = "/";
    }
    if (path[0] = '/') {
        path = "/" + path;
    }
    return MethodToString(method) + " " + path;
}
} // namespace rack::com