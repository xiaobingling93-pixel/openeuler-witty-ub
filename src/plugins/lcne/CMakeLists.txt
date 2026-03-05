find_package(tinyxml2 REQUIRED)
add_library(lcne STATIC lcne_module.cpp lcne_node_collector.cpp lcne_data_handler.cpp lcne_common.cpp lcne_topology.cpp)
target_include_directories(lcne PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lcne PUBLIC topology rack_http include tinyxml2::tinyxml2 witty_json)
