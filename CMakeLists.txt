cmake_minimum_required(VERSION 3.21)

project(
  chip8
  VERSION 0.1.0
  DESCRIPTION "CHIP-8 emulator core"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHIP8_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(chip8_warnings INTERFACE)
target_compile_options(
  chip8_warnings
  INTERFACE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion;-Wshadow;-Wold-style-cast;-Wcast-align;-Wnon-virtual-dtor;-Woverloaded-virtual;-Wdouble-promotion;-Wnull-dereference;-Wformat=2>"
)
target_compile_options(chip8_warnings INTERFACE "$<$<BOOL:${CHIP8_WARNINGS_AS_ERRORS}>:-Werror>")

add_library(chip8 src/emulator.cpp)
add_library(chip8::chip8 ALIAS chip8)
target_include_directories(
  chip8
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>)
target_link_libraries(chip8 PRIVATE chip8_warnings)

include(CTest)
if(BUILD_TESTING)
  add_library(chip8_doctest INTERFACE)
  target_include_directories(chip8_doctest SYSTEM INTERFACE third_party)

  add_executable(chip8_tests tests/main.cpp tests/emulator_test.cpp)
  target_link_libraries(chip8_tests PRIVATE chip8::chip8 chip8_doctest chip8_warnings)
  add_test(NAME chip8_tests COMMAND chip8_tests)
endif()

include(GNUInstallDirs)
install(TARGETS chip8 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
