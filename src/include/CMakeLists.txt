add_library(include STATIC node_data_def.cpp failure_def.cpp utils.cpp)
find_package(log4cplus QUIET)

if(NOT log4cplus_FOUND)
    # 如果 find_package 失败，尝试手动查找
    message(STATUS "find_package(log4cplus) failed, trying manual search...")
    
    # 尝试常见的 Linux 安装路径
    set(POSSIBLE_PATHS
        "/usr/local"
        "/usr"
        "/opt/log4cplus"
        "/home/${CMAKE_USER}/.local"
        "/usr/local/log4cplus"
    )
    
    foreach(PATH ${POSSIBLE_PATHS})
        if(EXISTS "${PATH}/include/log4cplus/logger.h")
            set(LOG4CPLUS_ROOT "${PATH}" CACHE PATH "log4cplus root directory" FORCE)
            set(LOG4CPLUS_INCLUDE_DIR "${PATH}/include" CACHE PATH "log4cplus include directory" FORCE)
            
            # 查找库文件（Linux 使用 .a 或 .so）
            if(EXISTS "${PATH}/lib/liblog4cplus.a")
                set(LOG4CPLUS_LIBRARIES "${PATH}/lib/liblog4cplus.a" CACHE FILEPATH "log4cplus library" FORCE)
            elseif(EXISTS "${PATH}/lib/liblog4cplus.so")
                set(LOG4CPLUS_LIBRARIES "${PATH}/lib/liblog4cplus.so" CACHE FILEPATH "log4cplus library" FORCE)
            elseif(EXISTS "${PATH}/lib64/liblog4cplus.a")
                set(LOG4CPLUS_LIBRARIES "${PATH}/lib64/liblog4cplus.a" CACHE FILEPATH "log4cplus library" FORCE)
            elseif(EXISTS "${PATH}/lib64/liblog4cplus.so")
                set(LOG4CPLUS_LIBRARIES "${PATH}/lib64/liblog4cplus.so" CACHE FILEPATH "log4cplus library" FORCE)
            endif()
            
            if(LOG4CPLUS_LIBRARIES)
                set(log4cplus_FOUND TRUE)
                message(STATUS "Found log4cplus at: ${PATH}")
                break()
            endif()
        endif()
    endforeach()
endif()

# 链接库
target_link_libraries(include PRIVATE ${LOG4CPLUS_LIBRARIES})

# 添加包含目录
target_include_directories(include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
