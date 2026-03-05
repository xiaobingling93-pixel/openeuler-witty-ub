cmake_minimum_required(VERSION 3.16)
project(rack_http LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(rack_http STATIC
    http/rack_http_server.cpp
    http/rack_http_server_handler.cpp
    http/rack_http_module.cpp
    http/rack_http_client.cpp
)
target_include_directories(rack_http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(httplib REQUIRED)
target_link_libraries(rack_http PUBLIC httplib::httplib)


add_executable(http_server_main
    http_server_main.cpp
)

target_include_directories(http_server_main PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(http_server_main
    PUBLIC
        rack_http
        include
)

# =============================================
# http client executable
# =============================================
add_executable(http_client_main
    http_client_main.cpp
)

find_package(tinyxml2 REQUIRED)
target_link_libraries(http_client_main PRIVATE rack_http tinyxml2::tinyxml2)


