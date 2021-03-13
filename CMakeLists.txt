cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

add_definitions(-DUNICODE)

if (WIN32)
  set(DEPS)
endif()

add_executable(main main.c)
foreach (dep ${DEPS})
  target_link_libraries(main ${dep})
endforeach()
