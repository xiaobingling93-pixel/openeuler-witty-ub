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
#include <regex>

#include "failure_def.h"

namespace failure::log {
    class LogTemplate {
    public:
        LogTemplate(const FailureMode& mode);

        std::optional<std::unordered_map<std::string, std::string>> Match(const std::string& line) const;
        std::optional<FailureEvent> CreateEvent(
            std::unordered_map<std::string, std::string>&& attributes,
            std::string&& line
        ) const;

    private:
        static std::string Escape(const std::string& str);

        void CreateRegexCaptor(const std::string& manifest);
        std::optional<std::unordered_map<std::string, std::string>> CaptureFields(const std::string& line) const;

    private:
        FailureMode mode_;
        std::regex pattern_;
        std::vector<std::string> fields_;
    };
}
