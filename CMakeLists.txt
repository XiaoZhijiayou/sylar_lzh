set(CMAKE_C_COMPILER /usr/bin/gcc)  # 替换为实际的 gcc 路径
set(CMAKE_CXX_COMPILER /usr/bin/g++)  # 替换为实际的 g++ 路径


cmake_minimum_required(VERSION 2.8)
project(sylar)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Werror -Wno-unused-function -Wno-deprecated -g -fPIC -fdiagnostics-color=always")
set(LIB_SRC
        sylar/log.cpp
    )
message(STATUS "LIB_SRC: ${LIB_SRC}")
add_library(sylar SHARED ${LIB_SRC})
# add_library(mysylar STATIC ${LIB_SRC}) # for static library
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar") # for static library

add_executable(test tests/test.cpp)
add_dependencies(test sylar)
target_link_libraries(test sylar)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


