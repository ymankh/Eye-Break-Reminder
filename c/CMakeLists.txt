cmake_minimum_required(VERSION 3.21)

project(BreakReminderC VERSION 0.1.0 LANGUAGES C RC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(break_reminder_c WIN32
    main.c
    break-reminder.rc
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

if(WIN32)
    target_link_libraries(break_reminder_c PRIVATE comctl32 shell32 gdi32)
endif()
