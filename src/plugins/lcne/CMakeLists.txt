## Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 # witty-ub is licensed under the Mulan PSL v2.
 # You can use this software according to the terms and conditions of the Mulan PSL v2.
 # You may obtain a copy of Mulan PSL v2 at:
 #     http://license.coscl.org.cn/MulanPSL2
 # THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 # IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 # PURPOSE.
## See the Mulan PSL v2 for more details.

find_package(tinyxml2 REQUIRED)
add_library(lcne STATIC lcne_module.cpp lcne_node_collector.cpp lcne_data_handler.cpp lcne_common.cpp lcne_topology.cpp)
target_include_directories(lcne PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lcne PUBLIC topology rack_http include tinyxml2::tinyxml2 witty_json)
