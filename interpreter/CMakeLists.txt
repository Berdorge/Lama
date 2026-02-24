cmake_minimum_required(VERSION 3.24)

project(lama-interpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")

add_library(lama-runtime STATIC IMPORTED)
set_target_properties(lama-runtime PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../runtime/runtime.a"
)

add_executable(lama-interpreter assertions.cpp bytefile.cpp frame.cpp interpreter.cpp main.cpp verifier.cpp)
target_link_libraries(lama-interpreter PRIVATE lama-runtime)
