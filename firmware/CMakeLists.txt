cmake_minimum_required(VERSION 3.16)

# This firmware needs the selected speech engine and its declared ESP-IDF
# dependencies. Avoid initializing unrelated networking/crypto subsystems.
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "2026.10.18")
project(retroreader)

