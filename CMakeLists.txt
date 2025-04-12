set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
cmake_minimum_required(VERSION 3.10)
project(sylar)
set(CMAKE_BUILD_TYPE Debug)
include(cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic   -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 查找 PkgConfig 模块
find_package(PkgConfig REQUIRED)

# 使用 pkg-config 查找 mysqlclient 的信息
pkg_search_module(MYSQL REQUIRED mysqlclient)

# 输出调试信息，验证找到的路径
message(STATUS "MySQL include directories: ${MYSQL_INCLUDE_DIRS}")
message(STATUS "MySQL libraries: ${MYSQL_LIBRARIES}")

include_directories(.)
include_directories(/usr/include)
link_directories(/usr/lib)


set(LIB_SRC
        sylar/db/mysql.cpp
        sylar/log.cpp
        sylar/util.cpp
        sylar/config.cpp
        sylar/thread.cpp
        sylar/fiber.cpp
        sylar/mutex.cpp
        sylar/scheduler.cpp
        sylar/iomanager.cpp
        sylar/timer.cpp
        sylar/hook.cpp
        sylar/fd_manager.cpp
        sylar/address.cpp
        sylar/socket.cpp
        sylar/bytearray.cpp
        sylar/http/http.cpp
        sylar/http/http_server.cpp
        sylar/http/http_parser.cpp
        sylar/streams/socket_stream.cpp
        sylar/http/http_session.cpp
        sylar/http/servlet.cpp
        sylar/tcp_server.cpp
        sylar/stream.cpp
        sylar/http/http_connection.cpp
)

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

message(STATUS "LIB_SRC: ${LIB_SRC}")
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #___FILE___
# add_library(mysylar STATIC ${LIB_SRC}) # for static library
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar") # for static library


set(LIBS
        sylar
        dl
        pthread
        yaml-cpp
        ${MYSQL_LIBRARIES}  # 添加 MySQL 库
)


sylar_add_executable(test "tests/test.cpp" sylar "${LIBS}")
sylar_add_executable(test_config "tests/test_config.cpp" sylar "${LIBS}")
sylar_add_executable(test_thread "tests/test_thread.cpp" sylar "${LIBS}")
sylar_add_executable(test_util "tests/test_util.cpp" sylar "${LIBS}")
sylar_add_executable(test_fiber "tests/test_fiber.cpp" sylar "${LIBS}")
sylar_add_executable(test_scheduler "tests/test_scheduler.cpp" sylar "${LIBS}")
sylar_add_executable(test_iomanager "tests/test_iomanager.cpp" sylar "${LIBS}")
sylar_add_executable(test_hook "tests/test_hook.cpp" sylar "${LIBS}")
sylar_add_executable(test_address "tests/test_address.cpp" sylar "${LIBS}")
sylar_add_executable(test_socket "tests/test_socket.cpp" sylar "${LIBS}")
sylar_add_executable(test_bytearray "tests/test_bytearray.cpp" sylar "${LIBS}")
sylar_add_executable(test_http "tests/test_http.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_parser "tests/test_http_parser.cpp" sylar "${LIBS}")
sylar_add_executable(test_tcp_server "tests/test_tcp_server.cpp" sylar "${LIBS}")
sylar_add_executable(echo_server "examples/echo_server.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_server "tests/test_http_server.cpp" sylar "${LIBS}")
sylar_add_executable(test_http_connection "tests/test_http_connection.cpp" sylar "${LIBS}")
sylar_add_executable(test_uri "tests/test_uri.cpp" sylar "${LIBS}")
sylar_add_executable(test_example "tests/test_example" sylar "${LIBS}")
sylar_add_executable(test_mysql "tests/test_mysql.cpp" sylar "${LIBS}")
sylar_add_executable(test_mysql_1 "tests/test_mysql_1.cpp" sylar "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


