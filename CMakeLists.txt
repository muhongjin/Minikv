cmake_minimum_required(VERSION 3.16)
project(MiniKV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(inc .)

add_executable(minikv
    src/minikv.cpp
    cli_main.cpp
)
