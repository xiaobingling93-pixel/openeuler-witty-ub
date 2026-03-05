#define MODULE_NAME "HTTP_MODULE"
#include "rack_http_module.h"
#include "rack_http_server.h"
#include "logger.h"

namespace rack::com {
RackResult HttpModule::Initialize()
{
    return RACK_OK;
}

void HttpModule::UnInitialize() {}

RackResult HttpModule::Start()
{
    if (!RackHttpServer::GetInstance().Start()) {
        LOG_ERROR << "http tcp server start failed!";
        return RACK_FAIL;
    }
    return RACK_OK;
}

void HttpModule::Stop()
{
    RackHttpServer::GetInstance().Stop();
    LOG_INFO << "http stop end";
}

}  // namespace rack::com