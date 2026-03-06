#define MODULE_NAME "LCNE"
#include <unordered_map>

#include "rack_error.h"
#include "lcne_module.h"
#include "logger.h"
#include "lcne_topology.h"
namespace lcne::module
{
    using namespace ubse::context;
    using namespace topology::node;
    RackResult LcneModule::Initialize()
    {
        lcneTopology = std::make_shared<lcne::topo::LcneTopology>();
        return RACK_OK;
    }
    RackResult LcneModule::Start()
    {
#ifdef ENABLE_DAEMON_FEATURE
        if (auto notifyRet = lcneTopoligy->SubTopolgyChanges() != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneModule::Start-Error: failed to subscribe topology changes";
            return RACK_FAIL;
        }
#endif
        if (auto topoRet = lcneTopology->CreateTopolgy() != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneModule::Start-Error: failed to create topology";
            return RACK_FAIL;
        }
#ifdef ENABLE_DAEMON_FEATURE
        if (auto regRet = lcneTopology->RegLinkNotifyHttpHandler() != LCNE_SUCCESS)
        {
            LOG_ERROR << "LcneModule::Start-Error: failed to start topology";
            return RACK_FAIL;
        }
// todo register handler for master
#endif
        return RACK_OK;
    }
    void LcneModule::UnInitialize()
    {
        return;
    }
    void LcneModule::Stop()
    {
        return;
    }
} // namespace lcne::module
