add_library(witty_json STATIC witty_json_module.cpp)
target_include_directories(witty_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(nlohmann_json REQUIRED)
target_link_libraries(witty_json PRIVATE nlohmann_json::nlohmann_json)
