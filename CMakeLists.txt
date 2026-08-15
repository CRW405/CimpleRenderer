cmake_minimum_required(VERSION 3.10)

project(CimpleRenderer C)



set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Ofast -march=native")

add_executable(CimpleRenderer
    src/main.c
    src/obj.c
    src/draw.c
    src/term_ops.c
    src/input.c
    src/render.c
)

target_link_libraries(CimpleRenderer m)
