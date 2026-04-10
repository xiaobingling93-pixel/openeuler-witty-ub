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

#include <vector>

#include "failure_def.h"
#include "log_collector.h"

namespace failure::log {
class LogCallstackCollector final : public LogCollector {
public:
    RackResult Initialize() override;
    void UnInitialize() override;
    RackResult Start() override;
    void Stop() override;

private:
    RackResult ParseOpencodeConn(const std::unordered_map<std::string, std::string> &argMap);
    RackResult ParseSrcPath(const std::unordered_map<std::string, std::string> &argMap);
    RackResult ParseCompileCommandsPath(const std::unordered_map<std::string, std::string> &argMap);
    RackResult ParseArgs();

    std::string BuildAnalysisSkillPrompt(const std::string &skill, const SkillInput &input,
                                         const std::string &component);
    std::string BuildAggregationSkillPrompt(const std::string &skill, const SkillInput &input);
    std::string BuildKeyfuncSkillPrompt(const std::string &skill, const SkillInput &input);

    RackResult CreateSession();
    RackResult WaitForSessionVisible() const;
    RackResult SendMessageAsync(const std::string &prompt);
    RackResult WaitForAssistantMessageCompleted() const;
    RackResult FetchLatestAssistantMessage() const;
    RackResult Run(const std::string &skill, const std::string &prompt);

private:
    static const std::vector<std::string> allComponents_;

    OpencodeConnection conn_;
    SkillInput input_;
    std::string sessionId_;
    int64_t promptSubmittedAtMs_ = 0;
};
} // namespace failure::log
