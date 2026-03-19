## Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 # witty-ub is licensed under the Mulan PSL v2.
 # You can use this software according to the terms and conditions of the Mulan PSL v2.
 # You may obtain a copy of Mulan PSL v2 at:
 #     http://license.coscl.org.cn/MulanPSL2
 # THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 # IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 # PURPOSE.
## See the Mulan PSL v2 for more details.

add_library(failure_log STATIC
    log_collector.cpp
    log_local_collector_module.cpp
    log_parser.cpp
    log_reader.cpp
    log_template.cpp
)
target_include_directories(failure_log PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
find_package(re2 QUIET CONFIG)

if(TARGET re2::re2)
    set(RE2_TARGET re2::re2)
    get_target_property(RE2_INCLUDE_DIR re2::re2 INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "Found re2 at: ${RE2_INCLUDE_DIR}")
elseif(TARGET re2)
    set(RE2_TARGET re2)
    get_target_property(RE2_INCLUDE_DIR re2 INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "Found re2 at: ${RE2_INCLUDE_DIR}")
else()
    message(STATUS "find_package(re2) failed, trying manual search...")
    find_path(RE2_INCLUDE_DIR NAMES re2/re2.h)
    find_library(RE2_LIBRARY NAMES re2)
    if(NOT RE2_INCLUDE_DIR OR NOT RE2_LIBRARY)
        message(FATAL_ERROR "re2 not found")
    endif()

    add_library(re2::re2 UNKNOWN IMPORTED)
    set_target_properties(re2::re2 PROPERTIES
        IMPORTED_LOCATION "${RE2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${RE2_INCLUDE_DIR}"
    )
    set(RE2_TARGET re2::re2)
    message(STATUS "Found re2 at: ${RE2_INCLUDE_DIR}")
endif()

target_link_libraries(failure_log
    PUBLIC
        include
        ubse_context
        ${RE2_TARGET}
)
find_package(jsoncpp REQUIRED)
target_link_libraries(failure_log PRIVATE jsoncpp_lib)
