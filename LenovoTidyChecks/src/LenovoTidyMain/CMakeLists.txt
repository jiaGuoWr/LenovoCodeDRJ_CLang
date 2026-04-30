# lenovo-clang-tidy: standalone executable that statically links all rule
# modules. Used on Windows where -load=plugin.dll is unreliable, but is also
# perfectly fine to use on Linux/macOS as a single-binary distribution.

add_executable(lenovo-clang-tidy main.cpp)

# LLVM 18.1.8's official Windows SDK omits the build-time generated
# `clang-tidy-config.h` that `ClangTidyForceLinker.h` includes. Provide our
# own copy via this directory.
target_include_directories(lenovo-clang-tidy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Order matters less for MSVC than GCC, but listing the user-facing modules
# first makes intent clear. clangTidyMain provides the actual entry point;
# the *Module libraries are referenced via ClangTidyForceLinker.h's anchor
# pattern so the linker keeps their static initializers.
target_link_libraries(lenovo-clang-tidy PRIVATE
    lenovo_tidy_static

    clangTidyMain
    clangTidy
    clangTidyUtils

    clangTidyAbseilModule
    clangTidyAlteraModule
    clangTidyAndroidModule
    clangTidyBoostModule
    clangTidyBugproneModule
    clangTidyCERTModule
    clangTidyConcurrencyModule
    clangTidyCppCoreGuidelinesModule
    clangTidyDarwinModule
    clangTidyFuchsiaModule
    clangTidyGoogleModule
    clangTidyHICPPModule
    clangTidyLLVMLibcModule
    clangTidyLLVMModule
    clangTidyLinuxKernelModule
    clangTidyMiscModule
    clangTidyModernizeModule
    clangTidyObjCModule
    clangTidyOpenMPModule
    clangTidyPerformanceModule
    clangTidyPortabilityModule
    clangTidyReadabilityModule
    clangTidyZirconModule
)

# On MSVC, force the linker to retain every object from lenovo_tidy_static so
# that the LenovoTidyModuleAnchorSource symbol (and through it the
# ClangTidyModuleRegistry::Add<> static instance) survives linking.
if(MSVC)
    target_link_options(lenovo-clang-tidy PRIVATE
        /WHOLEARCHIVE:lenovo_tidy_static
    )
endif()

install(TARGETS lenovo-clang-tidy RUNTIME DESTINATION bin)
