cmake_minimum_required(VERSION 3.15)
project(Matcha LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

file(GLOB SOURCES "src/*.cpp")
add_executable(
  matcha
  ${SOURCES}
)

target_include_directories(
  matcha PRIVATE
  include
)
