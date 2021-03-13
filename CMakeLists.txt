cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_definitions(-DUNICODE)

if (WIN32)
  set(DEPS)
endif()

add_executable(main main.c)
target_link_libraries(main ${CONAN_LIBS})
foreach (dep ${DEPS})
  target_link_libraries(main ${dep})
endforeach()
