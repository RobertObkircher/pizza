cmake_minimum_required(VERSION 3.13)
project(pizza C)

set(CMAKE_C_STANDARD 11)

add_executable(pizza src/main.c)

target_link_libraries(pizza png)