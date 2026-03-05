#find_package(tinyxml2 REQUIRED)
add_library(urma STATIC urma_module.cpp urma_topology.cpp)
target_include_directories(urma PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(urma PUBLIC include ubse_context witty_json)
