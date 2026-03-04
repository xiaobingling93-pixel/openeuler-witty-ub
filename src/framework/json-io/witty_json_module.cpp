#define MODULE_NAME "WITTY_JSON"
#include "witty_json_module.h"
#include "logger.h"
#include <iostream>
#include <memory>

namespace witty_json::module {
using namespace rack::module;
RackResult JsonModule::Initialize() {
  json_io = std::make_shared<WittyJson>();
  LOG_INFO << "JsonModule::Initialize-Info: module initialized";
  return RACK_OK;
}
void JsonModule::Uninitialize() { return; }
RackResult JsonModule::Start() { return RACK_OK; }
void JsonModule::Stop() { return; }
