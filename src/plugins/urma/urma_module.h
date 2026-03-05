#ifndef URMA_MODULE_H
#define URMA_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "urma_topology.h"
#include "witty_json.h"

namespace urma::module {
using namespace rack::module;
class URMAModule : public RackModule{
public:
    URMAModule()
    {
        dependencies.push_back(typeid(witty_json::module::JSONModule));
    }
    ~URMAModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
private:
    std::shared_ptr<urma::topo::URMATopology> urmaTopology;
};
}

#endif //URMA_MODULE_H