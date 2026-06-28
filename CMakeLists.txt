cmake_minimum_required(VERSION 3.22)

project(
  compiler
  VERSION "0.0.1"
  DESCRIPTION "My compiler"
  LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
