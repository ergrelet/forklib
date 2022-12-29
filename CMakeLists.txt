cmake_minimum_required(VERSION 3.12)

project(forklib
  VERSION 0.3.0
  LANGUAGES CXX C
)

option(FORKLIB_BUILD_SHARED_LIB "Build forklib as a shared library" ON)
option(FORKLIB_BUILD_TESTS "Build executables used for testing" OFF)
option(FORKLIB_FORCE_SHARED_CRT "Use shared (DLL) run-time lib" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(NOT FORKLIB_FORCE_SHARED_CRT AND MSVC)
  add_compile_options("/MT$<$<CONFIG:Debug>:d>")
endif()

add_subdirectory("src")
add_subdirectory("wow64ext")

if(FORKLIB_BUILD_TESTS)
  add_subdirectory("tests")
endif()
