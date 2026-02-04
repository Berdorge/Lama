cmake_minimum_required(VERSION 3.24)

project(lama-insnfreq-analysis LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")

add_executable(lama-insnfreq-analysis main.cpp analyzer.cpp ../byterun/byterun-lib.c)
