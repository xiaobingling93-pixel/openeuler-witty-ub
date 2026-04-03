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
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <json/json.h>

namespace failure {
enum class DataSourceOption {
    KERNEL,
    USER
};

enum class EventTypeOption {
    BIND,
    UNBIND,
    POST
};

using KeyFuncEventTypeMap = std::unordered_map<std::string, EventTypeOption>;
using KeyFuncRoleMap = std::unordered_map<std::string, std::string>;

static const KeyFuncEventTypeMap keyFuncEventTypeMap = {
    {"umq_ub_bind_inner_impl", EventTypeOption::BIND}, {"umq_ub_connect_jetty", EventTypeOption::BIND},
    {"umq_ub_unbind_impl", EventTypeOption::UNBIND},   {"umq_ub_post_tx", EventTypeOption::POST},
    {"umq_ub_poll_tx", EventTypeOption::POST},         {"umq_ub_post_rx_inner_impl", EventTypeOption::POST},
    {"umq_ub_poll_rx", EventTypeOption::POST},
};

static const KeyFuncRoleMap keyFuncRoleMap = {
    {"umq_ub_post_tx", "tx"},
    {"umq_ub_poll_tx", "tx"},
    {"umq_ub_post_rx_inner_impl", "rx"},
    {"umq_ub_poll_rx", "rx"},
};

struct PathCell {
    std::optional<std::string> podId;
    std::string path;

    PathCell() = default;
    PathCell(const std::optional<std::string> &podId, const std::string &path);
};

struct DataSource {
    DataSourceOption option;
    std::vector<PathCell> pathCells;
};

struct FailureMode {
    std::string component;
    std::string version;
    DataSource dataSource;
    bool isMultiline;
    std::string manifest;

    static FailureMode FromJson(const Json::Value &j);
};

struct FailureEvent {
    int64_t timestamp;
    std::string component;
    PathCell pathCell;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;

    Json::Value ToJson() const;
};

struct FailureMetadata {
    EventTypeOption eventType;
    std::optional<std::string> podId;
    std::string programName;
    std::string procId;
    int64_t timestamp;
    std::string localEid;
    std::string localJettyId;
    std::optional<std::string> remoteEid;
    std::optional<std::string> remoteJettyId;
    std::string text;
    std::optional<std::string> role;
    std::vector<const FailureEvent *> events;

    std::string funcName;

    Json::Value ToJson() const;
};

struct FailureEventQuery {
    int64_t startTime;
    int64_t endTime;
    std::unordered_set<EventTypeOption> eventTypes;
    std::unordered_set<std::string> podIds;
    std::unordered_set<std::string> localEids;
    std::unordered_set<std::string> jettyIds;

    bool Match(const FailureMetadata &metadata, bool podMode) const;
};

struct OpencodeConnection {
    std::string url;
    int port;
    std::optional<std::string> username;
    std::optional<std::string> passwd;
};

struct SkillInput {
    std::unordered_map<std::string, std::string> componentsPaths;
    std::unordered_map<std::string, std::string> compileCommandsPaths;
};

struct FuncNode {
    std::string id;
    std::string name;
    std::string component;
    std::string file;
};

struct CallEdge {
    std::string srcId;
    std::string dstId;
};

struct CallGraph {
    std::vector<FuncNode> nodes;
    std::vector<CallEdge> edges;

    // 加速查询
    std::unordered_map<std::string, size_t> nodeIndex;

    // 邻接表
    std::vector<std::vector<size_t>> downstreamEdges;
    std::vector<std::vector<size_t>> upstreamEdges;
};

struct RelevantFuncs {
    std::vector<FuncNode> upstreamFuncs;
    std::vector<FuncNode> downstreamFuncs;

    // 加速查询
    std::unordered_map<std::string, std::vector<size_t>> upstreamNameIndex;
    std::unordered_map<std::string, std::vector<size_t>> downstreamNameIndex;
    std::unordered_map<std::string, size_t> upstreamIdIndex;
    std::unordered_map<std::string, size_t> downstreamIdIndex;
};

std::optional<EventTypeOption> EventTypeOptionFromString(const std::string &str);
std::string EventTypeOptionToString(EventTypeOption opt);
void Split(std::vector<std::string> &out, const std::string &str, char delim, bool keepEmpty = false);
std::optional<int64_t> DatetimeStrToTimestamp(const std::string &datetimeStr);
std::optional<std::string> TimestampToDatetimeStr(int64_t timestamp, const std::string &format = "standard");
} // namespace failure
