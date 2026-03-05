#ifndef WITTY_JSON_MODULE_H
#define WITTY_JSON_MODULE_H
#include "rack_error.h"
#include "rack_module.h"
#include "witty_json.h"
namespace witty_json::module {
using namespace rack::module;
using namespace witty_json::io;
class JsonModule : public RackModule {
public:
  ~JsonModule() override = default;
  RackResult Initialize() override;
  void Uninitialize() override;
  RackResult Start() override;
  void Stop() override;
  JsonModule() = default;

  template <typename... Pairs>
  RackResult WriteVectorsTofile(const std::string &filename, Pairs &&...pairs) {
    auto ret =
        json_io->WriteVectorsTofile(filename, std::forward<Pairs>(pairs)...);
    if (ret != RACK_OK) {
      LOG_ERROR
          << "JsonModule::WriteVectorsTofile-Error: failed to write to file "
          << filename;
    }
    return ret;
  }
  template<typename T>
  auto GetJsonPair(const char *key, const std::vector<T> &vec){
    return std::make_pair(key, std::ref(vec));
  }

private:
  std::shared_ptr<WittyJson> json_io;
};

} // namespace witty_json::module
#endif // WITTY_JSON_MODULE_H
