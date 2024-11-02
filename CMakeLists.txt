cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(rpc VERSION 1.0 LANGUAGES CXX)

add_library(common common/stream.cpp)

add_executable(client client/main.cpp)
target_include_directories(client PUBLIC "./common")
target_link_libraries(client PUBLIC common)

add_executable(server server/main.cpp)
target_include_directories(server PUBLIC "./common")
target_link_libraries(server PUBLIC common)
