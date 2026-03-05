#ifndef RACK_ERROR_H
#define RACK_ERROR_H
#include <cstdint>

using RackResult = uint32_t;
const constexpr RackResult RACK_OK = 0;
const constexpr RackResult RACK_FAIL = 1;
#endif // RACK_ERROR_Hf