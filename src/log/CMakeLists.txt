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
target_link_libraries(failure_log
    PUBLIC
        include
        ubse_context
)
find_package(jsoncpp REQUIRED)
target_link_libraries(failure_log PRIVATE jsoncpp_lib)