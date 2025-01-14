cmake_minimum_required(VERSION 3.10)

project(allocators C)

include_directories(src)

add_executable(allocators main.c src/allocators_linear.c)
