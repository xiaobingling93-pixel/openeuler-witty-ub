#define MODULE_NAME "URMA"
#include <unordered_map>

#include "rack_error.h"
#include "urma_module.h"
#include "logger.h"
#include "ubse_context.h"
#include "urma_error.h"
#include "urma_topology.h"

namespace urma::module {
using namespace ubse::context;
RackResult URMAModule::Initialize()
{
    urmaTopology = std::make_shared<urma::topo::URMATopology>();
    return RACK_OK;
}
RackResult URMAModule::Start()
{
    LOG_DEBUG << "Execute URMAModule Start";
    UbseContext& g_rackContext = UbseContext::GetInstance();
    auto args = g_rackContext.GetTopoToolArgs();
    URMAResult ret = urmaTopology->CreateTopology(args);
    if(ret == URMA_FAIL){
        LOG_ERROR << "Execute URMAModule Start Error";
        return RACK_FAIL;
    }
    LOG_DEBUG << "Execute URMAModule Success";
    retur RACK_OK;
}
void URMAModule::UnInitialize()
{
    return;
}
void URMAModule::Stop()
{
    return;
}
} // namespace urma::module