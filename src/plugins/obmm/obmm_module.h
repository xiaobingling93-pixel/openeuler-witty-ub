#ifndef OBMM_MODULE_H
#define OBMM_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "mem_local_collector_module.h"
#include "obmm_mem_collector.h"
namespace obmm::module {
using namespace obmm::collector;
using namespace rack::module;
using namespace topology::mem;
class ObmmModule : public RackModule {
public:
    ObmmModule()
    {
        dependencies.push_back(typeid(MemLocalCollectorModule));
    }
    ~ObmmModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() oeverride;
private:
    std::shared_ptr<ObmmMemCollector> obmmCollector;
    std::shared_ptr<MemLocalCollectorModule> memLocalCollectorModule;
};
}
#endif // OBMM_MODULE_H