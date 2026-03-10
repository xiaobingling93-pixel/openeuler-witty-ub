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

#define MODULE_NAME "LOG"

#include "log_parser.h"

#include "logger.h"

namespace failure::log {
    void LogParser::AddFailureMode(const FailureMode& mode)
    {
        if (mode.isMultiline) {
            multiLineTemplates_.emplace_back(mode);
        } else {
            singleLineTemplates_.emplace_back(mode);
        }
    }

    std::optional<LogParser::TemplateEntry> LogParser::MatchMultiLineTemplate(const std::string& line) const
    {
        for (const LogTemplate& tmpl : multiLineTemplates_) {
            if (auto attributes = tmpl.Match(line)) {
                return std::make_pair(&tmpl, std::move(*attributes));
            }
        }
        return std::nullopt;
    }

    std::optional<LogParser::TemplateEntry> LogParser::MatchSingleLineTemplate(const std::string& line) const
    {
        for (const LogTemplate& tmpl : singleLineTemplates_) {
            if (auto attributes = tmpl.Match(line)) {
                return std::make_pair(&tmpl, std::move(*attributes));
            }
        }
        return std::nullopt;
    }
}
