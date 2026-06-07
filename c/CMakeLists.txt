cmake_minimum_required(VERSION 3.21)

project(BreakReminderC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(break_reminder_c
    src/main.c
)

target_include_directories(break_reminder_c
    PRIVATE
        include
)

if(MSVC)
    target_compile_options(break_reminder_c PRIVATE /W4 /permissive-)
else()
    target_compile_options(break_reminder_c PRIVATE -Wall -Wextra -Wpedantic)
endif()
