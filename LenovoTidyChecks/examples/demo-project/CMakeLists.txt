cmake_minimum_required(VERSION 3.20)
project(LenovoTidyDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(demo_bad  src/demo.cpp)
add_executable(demo_good src/demo_ok.cpp)
