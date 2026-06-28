cmake_minimum_required(VERSION 3.15)

project(Itch-Gen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../../bin/tools")

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(
  itch-gen
  ${SOURCES}
)

target_include_directories(
  itch-gen PRIVATE
  "${CMAKE_SOURCE_DIR}/include"
)
