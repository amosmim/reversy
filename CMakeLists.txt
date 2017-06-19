cmake_minimum_required(VERSION 3.6)
project(ex3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ex31.c ex32.c)

#add_executable(ex3.c ${SOURCE_FILES})
add_executable(ex3.c ex31.c)
add_executable(cli.c ex32.c)