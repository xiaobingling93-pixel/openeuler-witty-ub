#include "http/rack_http_client.h"
#include <httplib.h>

namespace rack::com {
RackHttpClient::RackHttpClient(std::string baseUrl) : baseUrl_(baseUrl)
{
}

RackComResult<RackHttpResponse> RackHttpClient::Do(const RackComContext &context, const RackHttpRequest &request)
{
    httplib::Client cli(baseUrl_);

    httplib::Headers headers;
    for (const auto &[k, v] : context.metadata) {
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
            }

            res = cli.Post(request.path, headers, request.body, content_type);
            break;
        }
        case RackHttpMethod::PUT: {
            std::string content_type = "text/plain";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                content_type = it->second;
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
        return RackComResult<RackHttpResponse>::Error(RackComError::UNAVAILABLE, "http request failed");
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