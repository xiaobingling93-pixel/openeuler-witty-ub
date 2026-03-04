add_library(ubse_context STATIC ubse_context.cpp)
target_include_directories(ubse_context PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ubse_context_tool STATIC ubse_context.cpp)
target_include_directories(ubse_context_tool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ubse_context_tool PUBLIC TOOL_MODE)