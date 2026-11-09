
cmake_minimum_required(VERSION 3.14)
project(JeniLang)

set(CMAKE_EXPORT_COMPILE_COMMANDS On)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")

add_subdirectory(vendor/mini.c)

file(GLOB_RECURSE SOURCES ${SOURCE_DIRECTORY}/*.cpp ${SOURCE_DIRECTORY}/*.hpp)

add_executable(Jeni_Compiler ${SOURCES})

target_include_directories(Jeni_Compiler PRIVATE
    ${SOURCE_DIRECTORY}
    ${CMAKE_SOURCE_DIR}/vendor/mini.c/include)

target_link_libraries(Jeni_Compiler PRIVATE
    asan
    mini.c
    mini.cc
)

target_compile_options(Jeni_Compiler PRIVATE
    -Wall
    -Wno-format
    -freflection
)

set_target_properties(Jeni_Compiler PROPERTIES
    RUNTIME_OUTPUT_NAME "jenic"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
