cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(rpc VERSION 1.0 LANGUAGES CXX)

add_library(common common/stream.cpp common/file_server.cpp common/file_client.cpp)
target_include_directories(common PRIVATE ${FUSE_INCLUDE_DIRS})


find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse3)

add_executable(client client/main.cpp)
target_include_directories(client PUBLIC "./common")
target_include_directories(client PRIVATE ${FUSE_INCLUDE_DIRS})
target_link_libraries(client PUBLIC common ${FUSE_LIBRARIES})

add_executable(server server/main.cpp)
target_include_directories(server PUBLIC "./common")
target_link_libraries(server PUBLIC common)


