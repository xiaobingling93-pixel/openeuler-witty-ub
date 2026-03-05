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