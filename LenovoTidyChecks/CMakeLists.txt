cmake_minimum_required(VERSION 3.20)

project(LenovoTidyChecks
    VERSION 0.2.0
    DESCRIPTION "Lenovo DRJ custom Clang-Tidy checks (C++ port of Roslyn analyzer rules)"
    LANGUAGES CXX C
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerOptions)

set(LENOVO_TIDY_REQUIRED_LLVM_MAJOR 18 CACHE STRING "Required LLVM major version")
set(LENOVO_TIDY_BUILD_TESTS ON CACHE BOOL "Build lit + unit tests")
set(LENOVO_TIDY_BUILD_EXAMPLES OFF CACHE BOOL "Build examples/demo-project")

include(FindLLVMDev)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src/LenovoTidyModule)
add_subdirectory(src/LenovoTidyMain)

if(LENOVO_TIDY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "--------------------------------------------------")
message(STATUS "LenovoTidyChecks ${PROJECT_VERSION}")
message(STATUS "  LLVM version : ${LLVM_PACKAGE_VERSION}")
message(STATUS "  LLVM dir     : ${LLVM_DIR}")
message(STATUS "  Clang dir    : ${Clang_DIR}")
message(STATUS "  Build tests  : ${LENOVO_TIDY_BUILD_TESTS}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "--------------------------------------------------")
