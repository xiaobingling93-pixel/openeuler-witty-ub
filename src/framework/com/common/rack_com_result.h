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

namespace rack::com {
    enum class RackComError {
        OK = 0,
        CANCELLED,
        DEADLINE_EXCEEDED,
        UNAVAILABLE,
        INVALID_ARGUMENT,
        INTERNAL,
    };

    template <typename T>
    struct RackComResult {
        RackComError code = RackComError::OK;
        std::string message;
        std::optional<T> value;

        bool Ok() const { return code == RackComError::OK; }

        static RackComResult<T> Ok(T v) {
            RackComResult<T> res;
            res.value = std::move(v);
            return res;
        }

        static RackComResult<T> Error(RackComError code, std::string message) {
            RackComResult<T> res;
            res.code = code;
            res.message = std::move(message);
            return res;
        }
    };
}