cmake_minimum_required(VERSION 3.22.1)
project(term-notes)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES main.c database.c term.c utils.c datatypes.c)

add_executable(term-notes ${SOURCE_FILES})
target_link_libraries(term-notes sqlite3)

