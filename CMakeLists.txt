# CStyleDS builds in C mode
enable_language(C)

file(GLOB _srcs *.c)
set(_target CStyleDS)

add_executable(${_target} ${_srcs})

# _typeof_ is a C23 feature. CMake maps c_std_23 to
# /std:clatest on MSVC (VS 2022 17.9+) and -std=c23 on GCC/Clang.
set_target_properties(${_target} PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(${_target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Macro arg-count overload forwards __VA_ARGS__ into another macro.
# (Same as in root preset, but activated for C.)
if(MSVC)
    target_compile_options(${_target} PRIVATE /Zc:preprocessor)
endif()

# Benchmarks comparing these containers against the C++ STL (C + C++, own target).
add_subdirectory(bench)
