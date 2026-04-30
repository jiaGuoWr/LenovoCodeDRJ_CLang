set(LENOVO_TIDY_SOURCES
    LenovoTidyModule.cpp
    common/StringUtils.cpp
    common/AstUtils.cpp
    common/OptionParser.cpp

    security/HardcodedSensitiveInfoCheck.cpp
    security/PathTraversalCheck.cpp
    security/SqlInjectionCheck.cpp
    security/UnsafeDeserializationCheck.cpp
    security/InsecureRandomCheck.cpp
    security/RegexDosCheck.cpp
    security/ResourceLeakCheck.cpp
    security/InsecureTempFileCheck.cpp
    security/RaceConditionCheck.cpp
    security/InsecureIpcCheck.cpp
    security/UnsafeDllLoadCheck.cpp

    localization/ChineseInCommentsCheck.cpp
    naming/NamingConventionCheck.cpp
    reliability/StackTraceInCatchCheck.cpp
    maintainability/CommentedOutCodeCheck.cpp
)

set(LENOVO_TIDY_HEADERS
    common/StringUtils.h
    common/AstUtils.h
    common/OptionParser.h

    security/HardcodedSensitiveInfoCheck.h
    security/PathTraversalCheck.h
    security/SqlInjectionCheck.h
    security/UnsafeDeserializationCheck.h
    security/InsecureRandomCheck.h
    security/RegexDosCheck.h
    security/ResourceLeakCheck.h
    security/InsecureTempFileCheck.h
    security/RaceConditionCheck.h
    security/InsecureIpcCheck.h
    security/UnsafeDllLoadCheck.h

    localization/ChineseInCommentsCheck.h
    naming/NamingConventionCheck.h
    reliability/StackTraceInCatchCheck.h
    maintainability/CommentedOutCodeCheck.h
)

add_library(LenovoTidyChecks MODULE
    ${LENOVO_TIDY_SOURCES}
    ${LENOVO_TIDY_HEADERS}
)

target_include_directories(LenovoTidyChecks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(LenovoTidyChecks
    PRIVATE
        clangTidy
        clangTidyUtils
        clangAST
        clangASTMatchers
        clangBasic
        clangLex
        clangTooling
        LLVMSupport
)

set_target_properties(LenovoTidyChecks PROPERTIES
    OUTPUT_NAME "LenovoTidyChecks"
    PREFIX "lib"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

if(APPLE)
    set_target_properties(LenovoTidyChecks PROPERTIES
        SUFFIX ".dylib"
        LINK_FLAGS "-Wl,-flat_namespace -Wl,-undefined,suppress"
    )
elseif(UNIX)
    set_target_properties(LenovoTidyChecks PROPERTIES SUFFIX ".so")
elseif(WIN32)
    set_target_properties(LenovoTidyChecks PROPERTIES SUFFIX ".dll")
endif()

add_library(lenovo_tidy_core STATIC
    common/StringUtils.cpp
    common/StringUtils.h
    common/AstUtils.cpp
    common/AstUtils.h
    common/OptionParser.cpp
    common/OptionParser.h
)
target_include_directories(lenovo_tidy_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(lenovo_tidy_core
    PUBLIC
        clangAST
        clangBasic
        clangLex
        LLVMSupport
)
set_target_properties(lenovo_tidy_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS LenovoTidyChecks
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Static-archive variant of the rule set. Linked into the lenovo-clang-tidy
# executable (see ../LenovoTidyMain/) so we do not depend on clang-tidy's
# plugin loader, which is broken on Windows builds of LLVM.
add_library(lenovo_tidy_static STATIC
    ${LENOVO_TIDY_SOURCES}
    ${LENOVO_TIDY_HEADERS}
)

target_include_directories(lenovo_tidy_static
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lenovo_tidy_static
    PUBLIC
        clangTidy
        clangTidyUtils
        clangAST
        clangASTMatchers
        clangBasic
        clangLex
        clangTooling
        LLVMSupport
)

set_target_properties(lenovo_tidy_static PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
