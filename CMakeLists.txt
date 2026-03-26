cmake_minimum_required(VERSION 3.30)
project(hexodus)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES "src/*.cpp")
add_executable(hexodus ${SOURCES})

target_compile_options(hexodus PRIVATE -Wall -Wextra -stdlib=libc++)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(hexodus PRIVATE -stdlib=libc++)
    target_link_options(hexodus PRIVATE -stdlib=libc++)
endif ()
