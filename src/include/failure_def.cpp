#include "failure_def.h"
#include <charconv>
#include <sstream>
#include "utils.h"

namespace failure {
    bool FailureEventQuery::Match(const FailureEvent& event) const {
        if(startTime && event.timestamp < startTime) {
            return false;
        }
        if(endTime && event.timestamp > endTime) {
            return false;
        }
        return true;
    }
    DataSourceOption DataSourceOptionFromString(const std::string& str) {
        if(str.empty()) {
            return DataSourceOption::UNKNOWN;
        }else if(str == "dmesg") {
            return DataSourceOption::KERNEL;
        }else {
            return DataSourceOption::USER;
        }
    }
    EventTypeOption EventTypeOptionFromString(const std::string& str) {

        if(str == "bind") {
            return EventTypeOption::BIND;
        }else if(str == "unbind") {
            return EventTypeOption::UNBIND;
        }else if(str == "post") {
            return EventTypeOption::POST;
        }else {
            return EventTypeOption::UNKNOWN;
        }
    }
    FailureMode FailureMode::FromJson(const nlohmann::json& json) {
        FailureMode mode;
        mode.component = json.at("component").get<std::string>();
        mode.version = json.at("version").get<std::string>();
        mode.isMultiline = json.at("is_multiline").get<bool>();
        mode.manifest = json.at("manifest").get<std::string>();
        std::vector<std::string> paths;
        if(json.at("log_path").is_string()) {
            paths.push_back(json.at("log_path").get<std::string>());
        }else {
            paths = json.at("log_path").get<std::vector<std::string>>();
        }
        DataSourceOption opt = DataSourceOptionFromString(paths[0]);
        mode.dataSource = { opt, std::move(paths)};
        return mode;
    }
    nlohmann::json FailureMode::ToJson() const {
        nlohmann::json json;
        if(auto time = utils::TimestampToDatetimeStr(timestamp)){
            json["time"] = *time;
        }else {
            json["time"] = "unknown";
        }
        j["component"] = component;
        j["path"] = path;
        j["text"] = text;
        j["attributes"] = attributes;
        return json;
    }
}
