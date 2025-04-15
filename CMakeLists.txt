set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(sylar)
set(CMAKE_BUILD_TYPE Debug)
include(cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic   -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

message(STATUS "CMake Toolchain File: ${CMAKE_TOOLCHAIN_FILE}")

# if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#   set(CMAKE_TOOLCHAIN_FILE "/home/li/project/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Toolchain file for vcpkg" FORCE)
# endif()


set(Abseil_DIR "/usr/lib/cmake/absl")
find_package(absl REQUIRED COMPONENTS 
    absl_base 
    absl_strings
    absl_any
    absl_flat_hash_map
    absl_flags
)
# 查找 PkgConfig 模块
# find_package(PkgConfig REQUIRED)

# 查找Zookeeper库的头文件路径
find_path(ZOOKEEPER_INCLUDE_DIR 
    NAMES zookeeper.h 
    PATHS /usr/local/include/zookeeper
)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()



# 查找Zookeeper库的路径
find_library(ZOOKEEPER_LIBRARY 
    NAMES zookeeper_mt 
    PATHS /usr/local/lib
)

# 检查是否找到了库和头文件
if(ZOOKEEPER_INCLUDE_DIR AND ZOOKEEPER_LIBRARY)
    message(STATUS "Found Zookeeper: ${ZOOKEEPER_LIBRARY}")
else()
    message(FATAL_ERROR "Could not find Zookeeper library")
endif()

# 使用 pkg-config 查找 mysqlclient 的信息
pkg_search_module(MYSQL REQUIRED mysqlclient)

# 输出调试信息，验证找到的路径
message(STATUS "Zookeeper include directories: ${ZOOKEEPER_INCLUDE_DIR}")
message(STATUS "Zookeeper libraries: ${ZOOKEEPER_LIBRARY}")

# 输出调试信息，验证找到的路径
message(STATUS "MySQL include directories: ${MYSQL_INCLUDE_DIRS}")
message(STATUS "MySQL libraries: ${MYSQL_LIBRARIES}")


# 查找 Protobuf 库
find_package(Protobuf REQUIRED)


# 输出调试信息，验证找到的路径
message(STATUS "----------------- Zookeeper Information -----------------")
message(STATUS "Zookeeper include directories: ${ZOOKEEPER_INCLUDE_DIR}")
message(STATUS "Zookeeper libraries: ${ZOOKEEPER_LIBRARY}")
message(STATUS "----------------- MySQL Information -----------------")
message(STATUS "MySQL include directories: ${MYSQL_INCLUDE_DIRS}")
message(STATUS "MySQL libraries: ${MYSQL_LIBRARIES}")
message(STATUS "----------------- Protobuf Information -----------------")
message(STATUS "Protobuf include directories: ${Protobuf_INCLUDE_DIRS}")
message(STATUS "Protobuf libraries: ${Protobuf_LIBRARIES}")
message(STATUS "---------------------------------------------------------")

# 包含 Zookeeper 头文件路径
include_directories(${ZOOKEEPER_INCLUDE_DIR})
include_directories(/usr/include/absl)
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
        sylar/env.cpp
        sylar/zk_client.cpp
        sylar/rpc/zkclient.cpp
        sylar/rpc/rpcheader.pb.cpp
        sylar/rpc/rpc_server.cpp
        sylar/rpc/rpc_provider.cpp
        sylar/rpc/rpc_controllerimpl.cpp
        sylar/rpc/rpc_channelimpl.cpp
        sylar/rpc/rpc_client.cpp
)

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

message(STATUS "LIB_SRC: ${LIB_SRC}")
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #___FILE___
# add_library(mysylar STATIC ${LIB_SRC}) # for static library
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar") # for static library
# set(PROTO_FILES examples/rpc/user.pb.cc)

#指定源代码文件
set(CALLEE_SOURCES
    examples/rpc/callee/userservice.cpp
    examples/rpc/user.pb.cpp  # 添加生成的 Protobuf C++ 文件
)

set(CALLER_SOURCES
    examples/rpc/caller/calluserservice.cpp
    examples/rpc/user.pb.cpp  # 添加生成的 Protobuf C++ 文件
)


include_directories(
    ${absl_INCLUDE_DIRS}
)

target_link_libraries(sylar
    ${LIBS}
    absl::base
    absl::strings
    absl::any
    absl::flat_hash_map
    absl::flags
    absl::log_internal_check_op 
)

set(LIBS
        sylar
        dl
        pthread
        yaml-cpp
        protobuf
        ${MYSQL_LIBRARIES}  # 添加 MySQL 
        ${ZOOKEEPER_LIBRARY} # 添加 Zookeeper 库
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
sylar_add_executable(test_example "tests/test_example.cpp" sylar "${LIBS}")
sylar_add_executable(test_mysql "tests/test_mysql.cpp" sylar "${LIBS}")
sylar_add_executable(test_mysql_1 "tests/test_mysql_1.cpp" sylar "${LIBS}")
sylar_add_executable(test_tcpserver_passure "tests/test_tcpserver_passure.cpp" sylar "${LIBS}")
sylar_add_executable(test_tcpserver_bench "tests/test_tcpserver_bench.cpp" sylar "${LIBS}")
# sylar_add_executable(test_rpc_caller "examples/rpc/caller/calluserservice.cpp" sylar "${LIBS}" examples/rpc/user.pb.cc)
# sylar_add_executable(test_rpc_callee "examples/rpc/callee/userservice.cpp" sylar "${LIBS}" examples/rpc/user.pb.cc)

# 设置生成的可执行文件
add_executable(callee ${CALLEE_SOURCES})
add_executable(caller ${CALLER_SOURCES})

# 链接所需的库
target_link_libraries(callee ${LIBS})
target_link_libraries(caller ${LIBS})



SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


