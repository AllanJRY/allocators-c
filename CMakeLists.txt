cmake_minimum_required(VERSION 3.10)

project(allocators C)

# Search all .c files within src recursively,
file(GLOB_RECURSE SRCS "src/*.c")

include_directories("src")

# Compiler flags
# -g O3 etc are added according to Release / Debug compilation
#add_definitions(-std=c99 -Wall)

# Build a executable target (1st param: Target name; 2nd: source files)
add_executable(${PROJECT_NAME} main.c ${SRCS})
