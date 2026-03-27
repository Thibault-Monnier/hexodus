cmake_minimum_required(VERSION 3.30)
project(hexodus)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(hexodus ${SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/src)

target_compile_options(hexodus PRIVATE
        -Wall -Wextra -Wpedantic

        # Optimization level
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:
        -O3
        >
        $<$<CONFIG:Debug>:
        -O0
        >

        # Debugging/Profiling Info
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:
        -g
        -fno-omit-frame-pointer
        >

        # Sanitizers
        $<$<CONFIG:Debug>:
        -fsanitize=address,undefined
        >
)

target_link_options(hexodus PRIVATE
        $<$<CONFIG:Debug>:
        -fsanitize=address,undefined
        >
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(hexodus PRIVATE -stdlib=libc++)
    target_link_options(hexodus PRIVATE -stdlib=libc++)
endif ()
