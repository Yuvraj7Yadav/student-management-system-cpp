cmake_minimum_required(VERSION 3.15)

# If compiler is not explicitly set, prefer modern 64-bit MinGW toolchain over legacy 32-bit MinGW
if(NOT DEFINED CMAKE_C_COMPILER AND EXISTS "C:/mingw64/bin/gcc.exe")
    set(CMAKE_C_COMPILER "C:/mingw64/bin/gcc.exe" CACHE FILEPATH "C compiler" FORCE)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER AND EXISTS "C:/mingw64/bin/g++.exe")
    set(CMAKE_CXX_COMPILER "C:/mingw64/bin/g++.exe" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(StudentManagementSystem LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check compiler suitability
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    message(FATAL_ERROR "GCC version ${CMAKE_CXX_COMPILER_VERSION} is too old. C++17 threading and Crow require GCC 9.0 or higher. Please use a modern MinGW-w64 toolchain (e.g. C:/mingw64).")
endif()

# Preprocessor definitions for Windows / Asio standalone
add_compile_definitions(ASIO_STANDALONE)
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

# Source files
set(SOURCES
    backend/main.cpp
    backend/database.cpp
    backend/student.cpp
    backend/course.cpp
    third_party/sqlite/sqlite3.c
)

add_executable(student_management_server ${SOURCES})

# Windows networking sockets libraries required by Crow/Asio
if(WIN32)
    target_link_libraries(student_management_server PRIVATE ws2_32 mswsock)
endif()

# Threading library
find_package(Threads REQUIRED)
target_link_libraries(student_management_server PRIVATE Threads::Threads)

# Linux dl library for SQLite3 dynamic loader
if(UNIX AND NOT APPLE)
    target_link_libraries(student_management_server PRIVATE ${CMAKE_DL_LIBS})
endif()

# Static linking for MinGW to eliminate DLL search order collisions (avoids 0xC0000139)
if(MINGW)
    target_link_options(student_management_server PRIVATE -static -static-libgcc -static-libstdc++)
endif()
