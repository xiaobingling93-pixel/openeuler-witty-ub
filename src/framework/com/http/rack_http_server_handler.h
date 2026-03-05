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