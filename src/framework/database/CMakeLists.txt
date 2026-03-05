add_library(database STATIC
    database_module.cpp
    database.cpp
)
target include directories(database PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(SQLite3 REQUIRED MODULE)
target_link_libraries(database PUBLIC SQLite::SQLite3)