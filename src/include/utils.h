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
#include <vector>

namespace utils {
    void Split(std::vector<std::string>& out, const std::string& str, char delim, bool keepEmpty = false);
    std::optional<int64_t> DatetimeStrToTimestamp(const std::string& datetimeStr);
    std::optional<std::string> TimestampToDatetimeStr(int64_t timestamp, const std::string& format = "standard");
}
