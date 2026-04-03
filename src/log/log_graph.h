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

#include <cstddef>
#include <unordered_set>

#include "rack_error.h"
#include "failure_def.h"

namespace failure::log {
using KeyFuncRelevanceMap = std::unordered_map<std::string, RelevantFuncs>;

class LogGraph final {
public:
    RackResult LoadCallstack();
    RackResult LoadKeyFunctions();
    RackResult InitKeyFuncRelevance();
    void FindNodesByName(const std::string &funcName, std::vector<FuncNode> &matchedNodes) const;
    void FindNeighborhood(const std::string &funcId, size_t upstreamLevel, size_t downstreamLevel,
                          std::vector<FuncNode> &upstreamFuncs, std::vector<FuncNode> &downstreamFuncs) const;
    const KeyFuncRelevanceMap &GetKeyFuncRelevanceMap() const;
    const KeyFuncEventTypeMap &GetKeyFuncEventTypeMap() const;
    const KeyFuncRoleMap &GetKeyFuncRoleMap() const;

private:
    void CollectUpstreamNodes(size_t start, size_t maxLevel, std::unordered_set<size_t> &selected) const;
    void CollectDownstreamNodes(size_t start, size_t maxLevel, std::unordered_set<size_t> &selected) const;
    RackResult ReadJson(Json::Value &root, const std::string &path) const;
    RackResult BuildGraph(const Json::Value &root, CallGraph &graph) const;
    RackResult InitKeyFuncMap(const Json::Value &root, KeyFuncEventTypeMap &keyFuncEventTypeMap,
                              KeyFuncRoleMap &keyFuncRoleMap);

    CallGraph graph_;
    KeyFuncRelevanceMap keyFuncRelevanceMap_;
    KeyFuncEventTypeMap keyFuncEventTypeMap_;
    KeyFuncRoleMap keyFuncRoleMap_;
};
} // namespace failure::log
