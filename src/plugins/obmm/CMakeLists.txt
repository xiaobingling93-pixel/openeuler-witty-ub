add_library(obmm STATIC obmm_module.cpp obmm_mem_collector.cpp)
target_include_directories(obmm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(obmm PUBLIC topology)