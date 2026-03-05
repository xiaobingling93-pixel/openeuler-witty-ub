#ifndef RACK_MODULE_H
#define RACK_MODULE_H
#include <memory>
#include <typeindex>
#include <vector>
#include "rack_error.h"

namespace rack::module {
class RackModule {
public:
    virtual ~RackModule() = default;
    virtual RackResult Initialize() = 0;
    virtual void Uninitialize() = 0;
    virtual RackResult Start() = 0;
    virtual void Stop() = 0;
    virtual void RegArgs(){};

    template <typename T>
    static std::shared_ptr<RackModule> CreateModule() {
        return std::make_shared<T>();
    }
    std::vector<std::type_index> GetDepencies() {
        return dependencies;
    }
protected:
    std::vector<std::type_index> dependencies;
};

}

#endif
