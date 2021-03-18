cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_definitions(-DUNICODE -D_CRT_SECURE_NO_WARNINGS)

if (WIN32)
  set(DEPS)
endif()
add_subdirectory(lib/mjson)
include_directories(lib/mjson)

include_directories(netbus/include)
include_directories(utils/include)
include_directories(database/include)
include_directories(proto)

file(GLOB_RECURSE utils "./utils/*.cc" "./utils/*.c")
file(GLOB_RECURSE netbus "./netbus/*.cc")
file(GLOB_RECURSE protob "./proto/*.cc")
file(GLOB_RECURSE db "./database/*.cc")
add_executable(main main.cc ${utils} ${db})
add_executable(test apps/test/main.cc ${netbus} ${utils} ${protob} ${db})
add_executable(client client/client.cc ${netbus} ${utils} ${protob})

target_link_libraries(main ${CONAN_LIBS})
target_link_libraries(test ${CONAN_LIBS} mjson)
target_link_libraries(client ${CONAN_LIBS})
foreach (dep ${DEPS})
  target_link_libraries(test ${dep})
  target_link_libraries(main ${dep})
endforeach()
