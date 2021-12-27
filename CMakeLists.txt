cmake_minimum_required(VERSION 3.12)

project(forklib
  VERSION 0.1.0
  LANGUAGES CXX
)

option(FORKLIB_RESTORE_STDIO "Restore standard handles in forked processes" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("src")
add_subdirectory("wow64ext")
