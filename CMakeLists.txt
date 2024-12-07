set(CMAKE_C_COMPILER /usr/bin/gcc)  # 替换为实际的 gcc 路径
set(CMAKE_CXX_COMPILER /usr/bin/g++)  # 替换为实际的 g++ 路径
cmake_minimum_required(VERSION 3.10)
project(sylar)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -Wall -O3 -Werror -Wno-unused-function -Wno-deprecated -g -rdynamic")

include_directories(.)
include_directories(/usr/include)
link_directories(/usr/lib)


set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
)
message(STATUS "LIB_SRC: ${LIB_SRC}")
add_library(sylar SHARED ${LIB_SRC})
# add_library(mysylar STATIC ${LIB_SRC}) # for static library
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar") # for static library


add_executable(test tests/test.cpp)
add_dependencies(test sylar)
target_link_libraries(test sylar)


add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


