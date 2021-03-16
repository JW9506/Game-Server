cmake_minimum_required(VERSION 3.0.0)
project(mjson VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

add_definitions(-DUNICODE)

file(GLOB_RECURSE files "./*.c")
add_library(mjson STATIC ${files})
