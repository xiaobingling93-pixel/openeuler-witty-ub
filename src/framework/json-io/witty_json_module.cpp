#define MODULE_NAME "WITTY_JSON"
#include "witty_json_module.h"
#include "logger.h"
#include <iostream>
#include <memory>

namespace witty_json::module
{
  using namespace rack::module;
  RackResult JSONModule::Initialize()
  {
    json_io = std::make_shared<WittyJson>();
    LOG_INFO << "JsonModule::Initialize-Info: module initialized";
    return RACK_OK;
  }
  void JSONModule::UnInitialize() { return; }
  RackResult JSONModule::Start() { return RACK_OK; }
  void JSONModule::Stop() { return; }
} // namespace witty_json::module