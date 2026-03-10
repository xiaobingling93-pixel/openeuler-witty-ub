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

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "failure_def.h"
#include "log_parser.h"

namespace failure::log {
    class LogReader {
    public:
        LogReader(DataSourceOption option, const PathCell& pathCell, int64_t startTime, int64_t endTime);
        ~LogReader();

        void CreateHandle();
        void DestroyHandle();
        void AddFailureMode(const FailureMode& mode);
        std::optional<FailureEvent> ReadOnce();

    private:
        std::optional<std::string> ReadNextLine();
        void ConfigureHandle(DataSourceOption option);

    private:
        static constexpr std::size_t READ_BUFFER_SIZE = 4096;

        PathCell pathCell_;
        int64_t startTime_;
        int64_t endTime_;

        FILE* handle_;
        std::function<FILE* (const std::string&)> opener_;
        std::function<void(FILE*)> closer_;

        std::optional<std::string> cachedLine_;
        std::array<char, READ_BUFFER_SIZE> readBuffer_{};
        std::string lineBuffer_;
        std::unique_ptr<LogParser> parser_;
    };
}
