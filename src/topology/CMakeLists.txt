add_library(topology STATIC
    node_global_collector_module.cpp
    node_local_collector_module.cpp
    node_collector.cpp
    ../include/node_data_def.cpp
)
target_include_directories(topology PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(topology PUBLIC include database ubse_context)