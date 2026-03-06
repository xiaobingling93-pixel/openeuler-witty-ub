/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * witty-ub is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "failure_def.h"
#include <charconv>
#include <sstream>
#include "utils.h"
#include <algorithm>

namespace failure {
    // 简单的 JSON 值提取辅助函数
    static std::string ExtractJsonValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) {
            return "";
        }
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) {
            return "";
        }
        pos++;
        
        // 跳过空白字符
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }
        
        if (pos >= json.size()) {
            return "";
        }
        
        // 字符串值
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            while (end != std::string::npos && json[end-1] == '\\') {
                end = json.find('"', end + 1);
            }
            if (end == std::string::npos) {
                return "";
            }
            return json.substr(pos, end - pos);
        }
        // 布尔值
        else if (json[pos] == 't' || json[pos] == 'f') {
            size_t end = json.find_first_of(",}]", pos);
            if (end == std::string::npos) {
                return json.substr(pos);
            }
            return json.substr(pos, end - pos);
        }
        // 数组
        else if (json[pos] == '[') {
            int depth = 1;
            size_t end = pos + 1;
            while (end < json.size() && depth > 0) {
                if (json[end] == '[') depth++;
                else if (json[end] == ']') depth--;
                end++;
            }
            return json.substr(pos, end - pos);
        }
        
        return "";
    }
    
    static bool ExtractJsonBool(const std::string& json, const std::string& key, bool defaultValue) {
        std::string value = ExtractJsonValue(json, key);
        if (value.empty()) {
            return defaultValue;
        }
        return value == "true";
    }
    
    static std::vector<std::string> ExtractJsonArray(const std::string& json, const std::string& key) {
        std::string arrayStr = ExtractJsonValue(json, key);
        std::vector<std::string> result;
        
        if (arrayStr.empty() || arrayStr[0] != '[') {
            return result;
        }
        
        size_t pos = 1;
        while (pos < arrayStr.size()) {
            // 跳过空白字符
            while (pos < arrayStr.size() && (arrayStr[pos] == ' ' || arrayStr[pos] == '\t' || arrayStr[pos] == '\n' || arrayStr[pos] == '\r' || arrayStr[pos] == ',')) {
                pos++;
            }
            
            if (pos >= arrayStr.size() || arrayStr[pos] == ']') {
                break;
            }
            
            if (arrayStr[pos] == '"') {
                pos++;
                size_t end = arrayStr.find('"', pos);
                while (end != std::string::npos && arrayStr[end-1] == '\\') {
                    end = arrayStr.find('"', end + 1);
                }
                if (end != std::string::npos) {
                    result.push_back(arrayStr.substr(pos, end - pos));
                    pos = end + 1;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        return result;
    }
    
    // 转义 JSON 字符串
    static std::string EscapeJsonString(const std::string& str) {
        std::string result;
        result.reserve(str.size() * 1.2);
        
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        result += buf;
                    } else {
                        result += c;
                    }
                    break;
            }
        }
        
        return result;
    }

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
    FailureMode FailureMode::FromJson(const std::string& jsonStr) {
        FailureMode mode;
        mode.component = ExtractJsonValue(jsonStr, "component");
        mode.version = ExtractJsonValue(jsonStr, "version");
        mode.isMultiline = ExtractJsonBool(jsonStr, "is_multiline", false);
        mode.manifest = ExtractJsonValue(jsonStr, "manifest");
        
        std::vector<std::string> paths = ExtractJsonArray(jsonStr, "log_path");
        if (paths.empty()) {
            std::string singlePath = ExtractJsonValue(jsonStr, "log_path");
            if (!singlePath.empty()) {
                paths.push_back(singlePath);
            }
        }
        
        DataSourceOption opt = DataSourceOption::UNKNOWN;
        if (!paths.empty()) {
            opt = DataSourceOptionFromString(paths[0]);
        }
        mode.dataSource = { opt, std::move(paths)};
        
        return mode;
    }
    
    std::string FailureEvent::ToJson() const {
        std::ostringstream oss;
        oss << "{";
        
        // time 字段
        oss << "\"time\": ";
        if(auto time = utils::TimestampToDatetimeStr(timestamp)){
            oss << "\"" << EscapeJsonString(*time) << "\"";
        } else {
            oss << "\"unknown\"";
        }
        
        // component 字段
        oss << ", \"component\": \"" << EscapeJsonString(component) << "\"";
        
        // path 字段
        oss << ", \"path\": \"" << EscapeJsonString(path) << "\"";
        
        // text 字段
        oss << ", \"text\": \"" << EscapeJsonString(text) << "\"";
        
        // attributes 字段
        oss << ", \"attributes\": {";
        bool first = true;
        for (const auto& [key, value] : attributes) {
            if (!first) {
                oss << ", ";
            }
            oss << "\"" << EscapeJsonString(key) << "\": \"" << EscapeJsonString(value) << "\"";
            first = false;
        }
        oss << "}";
        
        oss << "}";
        return oss.str();
    }
}