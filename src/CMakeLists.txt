project (Witty-UB VERSION 0.0.1)
add_compile_definitions(
    PROJECT_VERSION="${PROJECT_VERSION}"
)
add_subdirectory(include)
add_subdirectory(framework)
add_subdirectory(topology)
add_subdirectory(plugins)
add_subdirectory(log)

add_executable(copilot-worker
    ubse_main.cpp
    # framework/database/database_module.cpp
    # topology/node_global_collector_module.cpp
    # topology/node_local_collector_module.cpp
)
target_include_directories(copilot-worker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(copilot-worker PRIVATE database ubse_context topology lcne obmm failure_log rack_http)

add_executable(witty_ub_log
    witty_ub_log_main.cpp
)
target_include_directories(witty-ub-log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(witty-ub-log PRIVATE ubse_context_tool failure_log_tool)

option(ENABLE_DAEMON_FEATURE "Enable daemon feature" OFF)
option(ENABLE_TOOL_FEATURE "Enable tool feature" OFF)
# 如果启用了该选项，向编译器传递宏定义
if(ENABLE_DAEMON_FEATURE)
    add_compile_definitions(ENABLE_DAEMON_FEATURE)
endif()
if(ENABLE_TOOL_FEATURE)
    add_compile_definitions(ENABLE_TOOL_FEATURE)
endif()

add_executable(witty-ub-topo
    witty_ub_topo_main.cpp
)

target_include_directories(witty-ub-topo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(witty-ub-topo PRIVATE database ubse_context topology lcne obmm failure_log rack_http urma witty_json)
