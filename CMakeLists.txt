cmake_minimum_required(VERSION 3.22.1)
project(term-notes)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES main.c database.c term.c utils.c datatypes.c args.c)

add_executable(term-notes ${SOURCE_FILES})

if (WIN32) 
  target_link_libraries(term-notes PRIVATE ${CMAKE_SOURCE_DIR}/sqlite/sqlite3.lib)
  target_include_directories(term-notes PRIVATE sqlite)
endif()

if (UNIX) 
  target_link_libraries(term-notes PRIVATE sqlite3)
endif()

