# Locate LLVM / Clang development packages required to build clang-tidy plugins.
#
# Required version: LLVM 18.x (LENOVO_TIDY_REQUIRED_LLVM_MAJOR).
#
# On Ubuntu/Debian install via:
#     sudo apt install llvm-18-dev libclang-18-dev clang-tools-18 libclang-cpp18-dev
# On macOS (Homebrew):
#     brew install llvm@18
# On Windows:
#     Install the pre-built LLVM 18 package from https://llvm.org and set
#     LLVM_DIR / Clang_DIR env vars to the cmake/ folders before configuring.

# Default search hint: Debian/Ubuntu install path.
if(NOT LLVM_DIR AND EXISTS "/usr/lib/llvm-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}/cmake")
    set(LLVM_DIR "/usr/lib/llvm-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}/cmake")
endif()
if(NOT Clang_DIR AND EXISTS "/usr/lib/llvm-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}/lib/cmake/clang")
    set(Clang_DIR "/usr/lib/llvm-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}/lib/cmake/clang")
endif()

# We do major-version checking ourselves. Passing a version to find_package
# triggers LLVM's strict ConfigVersion matcher (only equal majors satisfy it),
# but the matcher behaviour varies subtly between distros, so we just locate
# the package then verify manually below.
find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

if(NOT LLVM_VERSION_MAJOR EQUAL LENOVO_TIDY_REQUIRED_LLVM_MAJOR)
    message(FATAL_ERROR
        "LenovoTidyChecks requires LLVM ${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}, "
        "but found ${LLVM_VERSION_MAJOR}. Install the matching LLVM dev package "
        "or override LENOVO_TIDY_REQUIRED_LLVM_MAJOR.")
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if(NOT LLVM_ENABLE_RTTI)
    if(MSVC)
        add_compile_options(/GR-)
    else()
        add_compile_options(-fno-rtti)
    endif()
endif()

find_program(LENOVO_CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR} clang-tidy
    DOC "Path to clang-tidy used for running lit tests"
)

find_program(LENOVO_LIT_EXECUTABLE
    NAMES lit llvm-lit
    DOC "Path to llvm-lit used for running regression tests"
)

find_program(LENOVO_FILECHECK_EXECUTABLE
    NAMES FileCheck FileCheck-${LENOVO_TIDY_REQUIRED_LLVM_MAJOR}
    DOC "Path to FileCheck used for pattern matching lit output"
)
