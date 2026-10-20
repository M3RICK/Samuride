# The game as the shared library Android's NativeActivity loads: libsamuride.so.
# Built by android/package.sh; the desktop builds use the Makefile.
cmake_minimum_required(VERSION 3.22)
project(samuride CXX)

set(CMAKE_CXX_STANDARD 20)
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# The game and the server core it hosts solo and versus runs with
file(GLOB SOURCES ${ROOT}/src/common/*.cpp ${ROOT}/src/server/*.cpp ${ROOT}/src/client/*.cpp
                  ${ROOT}/src/client/render/*.cpp)
list(REMOVE_ITEM SOURCES ${ROOT}/src/common/tests.cpp ${ROOT}/src/server/main.cpp)

add_library(samuride SHARED ${SOURCES})
target_include_directories(samuride PRIVATE ${ROOT}/src)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Main)
target_link_libraries(samuride PRIVATE SFML::Graphics SFML::Audio android log)

# SFML::Main's ANativeActivity_onCreate must end up in the library: it is what Android calls
target_link_libraries(samuride PUBLIC -Wl,--whole-archive SFML::Main -Wl,--no-whole-archive)

# Android 15 devices can use 16 KB pages
target_link_options(samuride PRIVATE "-Wl,-z,max-page-size=16384")
