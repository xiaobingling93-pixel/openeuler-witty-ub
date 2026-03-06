#ifndef HTTP_MODULE_H
#define HTTP_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"

namespace rack::com {
using namespace rack::module;
using namespace std;
class HttpModule : public RackModule {
public:
    ~HttpModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
    HttpModule() = default;
};
}
#endif // HTTP_MODULE_H