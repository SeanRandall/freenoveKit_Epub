cmake_minimum_required(VERSION 3.16)

# This firmware needs only the OpenEVV component and its declared ESP-IDF
# dependencies. Avoid initializing unrelated networking/crypto subsystems.
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(openevv_viability)
