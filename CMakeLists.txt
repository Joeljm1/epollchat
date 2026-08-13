cmake_minimum_required(VERSION 3.22)

project(
  epoll_chat
  VERSION "0.0.1"
  DESCRIPTION "epoll chat server"
  LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
