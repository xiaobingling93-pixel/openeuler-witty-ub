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

#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

namespace failure {
    enum class DataSourceOption{
        UNKNOWN,
        KERNEL,
        USER
    };
    enum class EventTypeOption{
        UNKNOWN,
        BIND,
        UNBIND,
        POST
    };

    struct DataSource{
        DataSourceOption option;
        std::vector<std::string> paths;
    };
    struct FailureMode{
        std::string component;
        std::string version;
        DataSource dataSource;
        bool isMultiline;
        std::string manifest;

        static FailureMode FromJson(const std::string& jsonStr);
    };

    struct FailureEvent{
        int64_t timestamp;
        std::string component;
        std::string path;
        std::string text;
        std::unordered_map<std::string, std::string> attributes;

        std::string ToJson() const;
    };
    struct FailureEventQuery{
        int64_t startTime;
        int64_t endTime;
        std::vector<EventTypeOption> eventTypes;
        std::vector<std::string> podIds;
        std::vector<std::string> localEids;
        std::vector<std::string> jettyIds;

        bool Match(const FailureEvent& event) const;
    };

    DataSourceOption DataSourceOptionFromString(const std::string& str);
    EventTypeOption EventTypeOptionFromString(const std::string& str);

}