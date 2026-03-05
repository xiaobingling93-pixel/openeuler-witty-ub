#ifndef DATABASE_MODULE_H
#define DATABASE_MODULE_H

#include <memory>
#include "rack_module.h"
#include "rack_error.h"
#include "database.h"

namespace database {
using namespace rack::module;
using namespace std;
class DatabaseModule : public RackModule {
public:
    ~DatabaseModule() override = default;
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;
    DatabaseModule() = default;
    shared_ptr<Database> GetDatabase();
private:
    shared_ptr<Database> db;
};
}
#endif // DATABASE_MODULE_H