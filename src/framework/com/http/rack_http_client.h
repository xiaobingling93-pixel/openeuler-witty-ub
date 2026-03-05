#include "common/rack_com_context.h"
#include "http/rack_http_client_handler.h"



namespace rack::com {
    class RackHttpClient : public RackHttpClientHandler {
    public:
        explicit RackHttpClient(std::string baseUrl);

        RackComResult<RackHttpResponse> Do(
            const RackComContext& context,
            const RackHttpRequest& request
        ) override;
    
    private:
        std::string baseUrl_;
    };
}