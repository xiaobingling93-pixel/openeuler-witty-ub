#pragma once

#include "common/rack_com_context.h"
#include "http/rack_http_types.h"

namespace rack::com {
    class RackHttpClientHandler {
    public:
        virtual ~RackHttpClientHandler() = default;

        virtual RackComResult<RackHttpResponse> Do(
            const RackComContext& context,
            const RackHttpRequest& request
        ) = 0;
    };
}