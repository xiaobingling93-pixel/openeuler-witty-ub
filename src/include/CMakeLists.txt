add_library(include STATIC node_data_def.cpp failure_def.cpp utils.cpp)

find_package(nlohmann_json REQUIRED)
find_package(glog REQUIRED)
target_link_libraries(include PRIVATE nlohmann_json::nlohmann_json glog::glog)
target_include_directories(include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
