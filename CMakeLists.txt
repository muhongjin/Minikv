cmake_minimum_required(VERSION 3.16)
project(MiniKV LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

include_directories(inc .)

# 命令行程序
add_executable(minikv
    src/minikv.cpp
    src/utils.cpp
    cli_main.cpp
)

# 测试程序
add_executable(minikv_test
    src/minikv.cpp
    src/utils.cpp
    minikv_test.cpp
)
