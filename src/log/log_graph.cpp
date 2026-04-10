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

#define MODULE_NAME "DIAGNOSIS"

#include "log_graph.h"

#include <json/json.h>

#include <algorithm>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <utility>

#include "failure_def.h"
#include "logger.h"

namespace failure::log {
constexpr const char *OVERALL_CALLSTACK_PATH = "/var/witty-ub/callstack-analysis/overall_callstack.json";
constexpr const char *KEY_FUNCTIONS_PATH = "/var/witty-ub/keyfunc-analysis/keyfunc.json";
constexpr const int UPSTREAM_LEVEL = 8;
constexpr const int DOWNSTREAM_LEVEL = 16;

void LogGraph::CollectUpstreamNodes(size_t start, size_t maxLevel, std::unordered_set<size_t> &selected) const
{
    std::queue<std::pair<size_t, size_t>> q;
    std::vector<int> visited(graph_.nodes.size(), -1);
    visited[start] = 0;
    q.push({start, 0});

    while (!q.empty()) {
        auto [cur, level] = q.front();
        q.pop();
        if (level >= maxLevel) {
            continue;
        }

        for (size_t parent : graph_.upstreamEdges[cur]) {
            if (visited[parent] != -1) {
                continue;
            }
            visited[parent] = static_cast<int>(level + 1);
            selected.insert(parent);
            q.push({parent, level + 1});
        }
    }
}

void LogGraph::CollectDownstreamNodes(size_t start, size_t maxLevel, std::unordered_set<size_t> &selected) const
{
    std::queue<std::pair<size_t, size_t>> q;
    std::vector<int> visited(graph_.nodes.size(), -1);
    visited[start] = 0;
    q.push({start, 0});

    while (!q.empty()) {
        auto [cur, level] = q.front();
        q.pop();
        if (level >= maxLevel) {
            continue;
        }

        for (size_t child : graph_.downstreamEdges[cur]) {
            if (visited[child] != -1) {
                continue;
            }
            visited[child] = static_cast<int>(level + 1);
            selected.insert(child);
            q.push({child, level + 1});
        }
    }
}

RackResult LogGraph::LoadCallstack()
{
    Json::Value root;
    RackResult ret = ReadJson(root, OVERALL_CALLSTACK_PATH);
    if (ret != RACK_OK) {
        return ret;
    }

    CallGraph loadedGraph;
    ret = BuildGraph(root, loadedGraph);
    if (ret != RACK_OK) {
        return ret;
    }

    graph_ = std::move(loadedGraph);
    return RACK_OK;
}

RackResult log::LogGraph::LoadKeyFunctions()
{
    Json::Value root;
    RackResult ret = ReadJson(root, KEY_FUNCTIONS_PATH);
    if (ret != RACK_OK) {
        return ret;
    }

    KeyFuncEventTypeMap loadedKeyFuncEventTypeMap;
    KeyFuncRoleMap loadedKeyFuncRoleMap;
    ret = InitKeyFuncMap(root, loadedKeyFuncEventTypeMap, loadedKeyFuncRoleMap);
    if (ret != RACK_OK) {
        return ret;
    }

    keyFuncEventTypeMap_ = std::move(loadedKeyFuncEventTypeMap);
    keyFuncRoleMap_ = std::move(loadedKeyFuncRoleMap);
    return RACK_OK;
}

RackResult LogGraph::InitKeyFuncRelevance()
{
    for (auto [funcName, _] : keyFuncEventTypeMap_) {
        std::vector<FuncNode> matchedNodes;
        FindNodesByName(funcName, matchedNodes);
        RelevantFuncs relevantFuncs;
        for (const FuncNode &node : matchedNodes) {
            FindNeighborhood(node.id, UPSTREAM_LEVEL, DOWNSTREAM_LEVEL, relevantFuncs.upstreamFuncs,
                             relevantFuncs.downstreamFuncs);
        }
        relevantFuncs.upstreamNameIndex.reserve(relevantFuncs.upstreamFuncs.size());
        relevantFuncs.downstreamNameIndex.reserve(relevantFuncs.downstreamFuncs.size());
        relevantFuncs.upstreamIdIndex.reserve(relevantFuncs.upstreamFuncs.size());
        relevantFuncs.downstreamIdIndex.reserve(relevantFuncs.downstreamFuncs.size());
        for (size_t i = 0; i < relevantFuncs.upstreamFuncs.size(); ++i) {
            const auto &func = relevantFuncs.upstreamFuncs[i];
            relevantFuncs.upstreamNameIndex[func.name].push_back(i);
            relevantFuncs.upstreamIdIndex.emplace(func.id, i);
        }
        for (size_t i = 0; i < relevantFuncs.downstreamFuncs.size(); ++i) {
            const auto &func = relevantFuncs.downstreamFuncs[i];
            relevantFuncs.downstreamNameIndex[func.name].push_back(i);
            relevantFuncs.downstreamIdIndex.emplace(func.id, i);
        }
        keyFuncRelevanceMap_.emplace(funcName, std::move(relevantFuncs));
    }
    return RACK_OK;
}

void LogGraph::FindNodesByName(const std::string &funcName, std::vector<FuncNode> &matchedNodes) const
{
    if (graph_.nodes.empty()) {
        LOG_WARN << "callstack graph is empty";
        return;
    }

    matchedNodes.clear();
    for (const auto &node : graph_.nodes) {
        if (node.name == funcName) {
            matchedNodes.push_back(node);
        }
    }
}

RackResult LogGraph::ReadJson(Json::Value &root, const std::string &path) const
{
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR << "failed to open callstack file: " << path;
        return RACK_FAIL;
    }

    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        LOG_ERROR << "failed to parse callstack json, path=" << path << ", err=" << errs;
        return RACK_FAIL;
    }
    if (!root.isObject()) {
        LOG_ERROR << "invalid callstack json root, path=" << path;
        return RACK_FAIL;
    }
    return RACK_OK;
}

RackResult LogGraph::BuildGraph(const Json::Value &root, CallGraph &graph) const
{
    const Json::Value *nodes = nullptr;
    const Json::Value *edges = nullptr;
    if (ValidateGraphRoot(root, nodes, edges) != RACK_OK) {
        return RACK_FAIL;
    }

    graph = {};
    if (BuildNodes(*nodes, graph) != RACK_OK) {
        return RACK_FAIL;
    }
    if (BuildEdges(*edges, graph) != RACK_OK) {
        return RACK_FAIL;
    }
    return RACK_OK;
}

RackResult LogGraph::ValidateGraphRoot(const Json::Value &root, const Json::Value *&nodes,
                                       const Json::Value *&edges) const
{
    nodes = &root["nodes"];
    edges = &root["edges"];
    if (!nodes->isArray() || !edges->isArray()) {
        LOG_ERROR << "invalid callstack json format, nodes_is_array=" << nodes->isArray()
                  << ", edges_is_array=" << edges->isArray();
        return RACK_FAIL;
    }
    return RACK_OK;
}

RackResult LogGraph::BuildNodes(const Json::Value &nodes, CallGraph &graph) const
{
    graph.nodes.reserve(nodes.size());
    for (Json::ArrayIndex i = 0; i < nodes.size(); ++i) {
        const Json::Value &node = nodes[i];
        if (!node.isObject()) {
            LOG_ERROR << "invalid node item at index " << i;
            return RACK_FAIL;
        }

        const std::string id = node["id"].asString();
        if (id.empty()) {
            LOG_ERROR << "node id is empty at index " << i;
            return RACK_FAIL;
        }
        if (graph.nodeIndex.find(id) != graph.nodeIndex.end()) {
            LOG_ERROR << "duplicated node id: " << id;
            return RACK_FAIL;
        }

        FuncNode funcNode;
        funcNode.id = id;
        funcNode.name = node["name"].asString();
        funcNode.component = node["component"].asString();
        funcNode.file = node["file"].asString();

        graph.nodeIndex[id] = graph.nodes.size();
        graph.nodes.emplace_back(std::move(funcNode));
    }

    graph.downstreamEdges.resize(graph.nodes.size());
    graph.upstreamEdges.resize(graph.nodes.size());
    return RACK_OK;
}

RackResult LogGraph::BuildEdges(const Json::Value &edges, CallGraph &graph) const
{
    graph.edges.reserve(edges.size());
    for (Json::ArrayIndex i = 0; i < edges.size(); ++i) {
        const Json::Value &edge = edges[i];
        if (!edge.isObject()) {
            LOG_ERROR << "invalid edge item at index " << i;
            return RACK_FAIL;
        }

        const std::string srcId = edge["src"].asString();
        const std::string dstId = edge["dst"].asString();
        if (srcId.empty() || dstId.empty()) {
            LOG_WARN << "skip edge with empty src/dst at index " << i;
            continue;
        }

        const auto srcIt = graph.nodeIndex.find(srcId);
        const auto dstIt = graph.nodeIndex.find(dstId);
        if (srcIt == graph.nodeIndex.end() || dstIt == graph.nodeIndex.end()) {
            LOG_WARN << "skip dangling edge at index " << i << ", src=" << srcId << ", dst=" << dstId;
            continue;
        }

        graph.edges.push_back({srcId, dstId});
        graph.downstreamEdges[srcIt->second].push_back(dstIt->second);
        graph.upstreamEdges[dstIt->second].push_back(srcIt->second);
    }
    return RACK_OK;
}

RackResult log::LogGraph::InitKeyFuncMap(const Json::Value &root, KeyFuncEventTypeMap &keyFuncEventTypeMap,
                                         KeyFuncRoleMap &keyFuncRoleMap)
{
    const Json::Value &functions = root["functions"];
    if (!functions.isArray()) {
        LOG_ERROR << "invalid keyfunc json format, functions_is_array=" << functions.isArray();
        return RACK_FAIL;
    }

    keyFuncEventTypeMap.clear();
    keyFuncRoleMap.clear();
    keyFuncEventTypeMap.reserve(functions.size());
    keyFuncRoleMap.reserve(functions.size());

    for (Json::ArrayIndex i = 0; i < functions.size(); ++i) {
        const Json::Value &function = functions[i];
        if (!function.isObject()) {
            LOG_ERROR << "invalid function item at index " << i;
            return RACK_FAIL;
        }

        const std::string funcName = function["name"].asString();
        const std::string domain = function["domain"].asString();
        if (funcName.empty() || domain.empty()) {
            LOG_ERROR << "invalid function item at index " << i << ", func_name=" << funcName << ", domain=" << domain;
            return RACK_FAIL;
        }

        EventTypeOption eventType;
        if (domain == "bind") {
            eventType = EventTypeOption::BIND;
        } else if (domain == "unbind") {
            eventType = EventTypeOption::UNBIND;
        } else if (domain == "post" || domain == "poll") {
            eventType = EventTypeOption::POST;
        } else {
            LOG_ERROR << "invalid domain in keyfunc json, index=" << i << ", domain=" << domain;
            return RACK_FAIL;
        }
        keyFuncEventTypeMap[funcName] = eventType;

        if (eventType == EventTypeOption::POST) {
            if (funcName.find("tx") != std::string::npos) {
                keyFuncRoleMap[funcName] = "tx";
            } else if (funcName.find("rx") != std::string::npos) {
                keyFuncRoleMap[funcName] = "rx";
            } else {
                keyFuncRoleMap[funcName] = "null";
            }
        }
    }

    return RACK_OK;
}

void LogGraph::FindNeighborhood(const std::string &funcId, size_t upstreamLevel, size_t downstreamLevel,
                                std::vector<FuncNode> &upstreamFuncs, std::vector<FuncNode> &downstreamFuncs) const
{
    if (graph_.nodes.empty()) {
        LOG_WARN << "callstack graph is empty";
        return;
    }

    const auto startIt = graph_.nodeIndex.find(funcId);
    if (startIt == graph_.nodeIndex.end()) {
        LOG_WARN << "function id not found in graph: " << funcId;
        return;
    }
    const size_t start = startIt->second;

    std::unordered_set<size_t> upstreamIndices;
    std::unordered_set<size_t> downstreamIndices;

    CollectUpstreamNodes(start, upstreamLevel, upstreamIndices);
    CollectDownstreamNodes(start, downstreamLevel, downstreamIndices);

    std::vector<size_t> sortedUpstreamIndices;
    sortedUpstreamIndices.reserve(upstreamIndices.size());
    for (size_t idx : upstreamIndices) {
        sortedUpstreamIndices.push_back(idx);
    }
    std::sort(sortedUpstreamIndices.begin(), sortedUpstreamIndices.end());

    std::vector<size_t> sortedDownstreamIndices;
    sortedDownstreamIndices.reserve(downstreamIndices.size());
    for (size_t idx : downstreamIndices) {
        sortedDownstreamIndices.push_back(idx);
    }
    std::sort(sortedDownstreamIndices.begin(), sortedDownstreamIndices.end());

    for (size_t idx : sortedUpstreamIndices) {
        upstreamFuncs.push_back(graph_.nodes[idx]);
    }
    for (size_t idx : sortedDownstreamIndices) {
        downstreamFuncs.push_back(graph_.nodes[idx]);
    }
}

const KeyFuncRelevanceMap &LogGraph::GetKeyFuncRelevanceMap() const
{
    return keyFuncRelevanceMap_;
}

const KeyFuncEventTypeMap &log::LogGraph::GetKeyFuncEventTypeMap() const
{
    return keyFuncEventTypeMap_;
}

const KeyFuncRoleMap &log::LogGraph::GetKeyFuncRoleMap() const
{
    return keyFuncRoleMap_;
}
} // namespace failure::log
