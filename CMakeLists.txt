set(CMAKE_C_COMPILER /usr/bin/gcc)  # 替换为实际的 gcc 路径
set(CMAKE_CXX_COMPILER /usr/bin/g++)  # 替换为实际的 g++ 路径
cmake_minimum_required(VERSION 3.10)
project(sylar)

include(cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/usr/include)
link_directories(/usr/lib)


set(LIB_SRC
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        sylar/thread.cpp
        sylar/fiber.cpp
        sylar/mutex.cpp
        sylar/scheduler.cpp
)
message(STATUS "LIB_SRC: ${LIB_SRC}")
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #___FILE___
# add_library(mysylar STATIC ${LIB_SRC}) # for static library
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar") # for static library




set(LIBS
        sylar
        pthread
        yaml-cpp
)

add_executable(test tests/test.cpp)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test) #___FILE___
target_link_libraries(test ${LIBS})


add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config) #___FILE___
target_link_libraries(test_config  ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
force_redefine_file_macro_for_sources(test_thread) #___FILE___
target_link_libraries(test_thread  ${LIBS})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
force_redefine_file_macro_for_sources(test_util) #___FILE___
target_link_libraries(test_util  ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber sylar)
force_redefine_file_macro_for_sources(test_fiber) #___FILE___
target_link_libraries(test_fiber  ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler sylar)
force_redefine_file_macro_for_sources(test_scheduler) #___FILE___
target_link_libraries(test_scheduler  ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


