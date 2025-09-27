cmake_minimum_required(VERSION 3.31)
project(Etatidem)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

add_library(rendrer STATIC
    src/Renderer.cpp
    src/Renderer.h
    src/config.h
    src/lib/stb/stb_truetype.h
    src/lib/stb/stb_truetype.c
)

add_executable(Etatidem src/main.cpp)

target_include_directories(Etatidem PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(Etatidem PRIVATE ${SDL2_LIBRARIES})


#
# tests
#

# (executable)
add_executable(tests
    src/specs/Renderer_test.cpp
    src/specs/Tests_main.cpp
)
target_include_directories(tests PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${SDL2_LIBRARIES} rendrer)


enable_testing()
add_test(NAME Tests COMMAND tests)
