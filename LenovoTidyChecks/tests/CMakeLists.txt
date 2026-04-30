include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(unit)

set(LENOVO_LIT_PLUGIN_PATH "$<TARGET_FILE:LenovoTidyChecks>")
set(LENOVO_LIT_CLANG_TIDY  "${LENOVO_CLANG_TIDY_EXECUTABLE}")
set(LENOVO_LIT_FILECHECK   "${LENOVO_FILECHECK_EXECUTABLE}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY
)

if(LENOVO_LIT_EXECUTABLE AND LENOVO_CLANG_TIDY_EXECUTABLE
   AND LENOVO_FILECHECK_EXECUTABLE)
    add_test(
      NAME lenovo-tidy-lit
      COMMAND ${LENOVO_LIT_EXECUTABLE}
              -v
              --path ${CMAKE_CURRENT_BINARY_DIR}
              ${CMAKE_CURRENT_SOURCE_DIR}/checkers
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(lenovo-tidy-lit PROPERTIES
        ENVIRONMENT "LENOVO_TIDY_PLUGIN=$<TARGET_FILE:LenovoTidyChecks>"
        LABELS "lit;integration"
    )
    message(STATUS "  lit runner  : ${LENOVO_LIT_EXECUTABLE}")
else()
    message(WARNING
        "lit / clang-tidy / FileCheck not found; regression tests will be skipped. "
        "Install llvm-18-dev + clang-tools-18 (Linux) or LLVM 18 full package (Win/Mac).")
endif()
