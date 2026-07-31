# Timed benchmarks: CStyleDS C containers vs the C++ standard library.
#
# One executable, two languages: bench_c.c is compiled as C23 (it includes the
# container headers, which are not valid C++), main.cpp as C++20 (the STL
# baselines + the timing harness). They are linked together; the C interface in
# bench.h is the only thing that crosses the boundary. No external dependencies.
set(_target CStyleDS_bench)

add_executable(${_target}
    main.cpp
    bench_c.c
)

# The container headers live one directory up (../CVec.h, ../CMat.h, ...).
target_include_directories(${_target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set_target_properties(${_target} PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# The container macros use arg-count overloading (__VA_ARGS__ forwarded into
# another macro); MSVC needs the conforming preprocessor for the C sources
# (same flag the CStyleDS target sets). /EHsc restores C++ exception unwinding
# for the STL baselines: the root preset sets CMAKE_CXX_FLAGS explicitly, which
# drops CMake's default /EHsc (otherwise <chrono>/containers warn C4530).
if(MSVC)
    target_compile_options(${_target} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:/Zc:preprocessor>
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
endif()
