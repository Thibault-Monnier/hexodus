cmake_minimum_required(VERSION 4.1)
project(hexodus)

set(CMAKE_CXX_STANDARD 20)

add_executable(hexodus src/main.cpp)

target_compile_options(hexodus PRIVATE -Wall -Wextra -stdlib=libc++)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(hexodus PRIVATE _LIBCPP_REMOVE_TRANSITIVE_INCLUDES)
    target_compile_options(hexodus PRIVATE -stdlib=libc++)
    target_link_options(hexodus PRIVATE -stdlib=libc++)
endif ()
