cmake_minimum_required(VERSION 3.10)

project(
  beeper
  VERSION 1.0.0
  DESCRIPTION "The Beeper library"
  LANGUAGES C
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
  message(STATUS "Build type not specified, using Debug.")
endif()

option(BP_32BIT "32-bit mode." OFF)

add_library(
  # Output.
  beeper
  # Type.
  STATIC
  # Source.
  src/beeper.c
)

if(BP_32BIT)
  set_target_properties(beeper PROPERTIES COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32")
endif()

target_compile_options(
  beeper PRIVATE
  -O3 -Weverything -Wno-reserved-identifier -Wno-padded -Wno-declaration-after-statement -Wno-used-but-marked-unused -Wno-format-nonliteral
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  target_compile_options(
    beeper PRIVATE
    -g3
  )
endif()

file(READ src/shared.h FILE_SHARED_H)
configure_file(src/beeper.h.in beeper.h)
