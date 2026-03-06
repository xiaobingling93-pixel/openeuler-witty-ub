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