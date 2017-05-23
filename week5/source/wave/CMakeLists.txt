cmake_minimum_required(VERSION 3.7)
project(wave)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        wave.c
        waveio.c
        waveio.h)

add_executable(wave ${SOURCE_FILES})