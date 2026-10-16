# Builds the C replacement TV_ENG32.DLL (tv_src/) and the cgrm_spk test program.
#
# Needs a 32-bit MinGW toolchain (winlibs mingw32 has gcc, cmake, ninja,
# windres and nm) and Python 3 with pefile:
#
#   set PATH=C:\winlibs\mingw32\bin;%PATH%
#   cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
#   cmake --build build
#
# Release: `cmake --install build --strip` puts the stripped TV_ENG32.DLL,
# cgrm_spk.exe and quickguide.txt in dist/ (--prefix <dir> for another folder).
#
# With -DTV_BUILD_DLL=OFF only cgrm_spk is built (no Python or original DLL
# needed); put a TV_ENG32.DLL beside it to run it.
#
# The tables and resources the DLL carries are extracted from an original
# TV_ENG32.DLL at build time (TV_DLL below); nothing extracted is committed.
cmake_minimum_required(VERSION 3.20)
project(TruVoice C CXX ASM)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "TV_ENG32.DLL is 32-bit: use a 32-bit MinGW (e.g. C:/winlibs/mingw32)")
endif()

option(TV_BUILD_DLL "Build the C TV_ENG32.DLL from tv_src" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/dist CACHE PATH "Release folder" FORCE)
endif()

# --- cgrm_spk: loads TV_ENG32.DLL from its own folder -----------------------------
add_executable(cgrm_spk cgrm_spk.cpp argparse.c parse.c)
set_source_files_properties(argparse.c parse.c PROPERTIES LANGUAGE CXX)
target_compile_options(cgrm_spk PRIVATE -std=c++0x -fpermissive)
target_link_options(cgrm_spk PRIVATE -static)
install(TARGETS cgrm_spk RUNTIME DESTINATION .)
install(FILES quickguide.txt DESTINATION .)

if(NOT TV_BUILD_DLL)
  return()
endif()
add_dependencies(cgrm_spk TV_ENG32)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(WINDRES windres REQUIRED)
set(TV_DLL "${CMAKE_SOURCE_DIR}/old/TV_ENG32.DLL" CACHE FILEPATH "Original TV_ENG32.DLL the data is extracted from")

set(TVS   ${CMAKE_SOURCE_DIR}/tv_src)
set(BE    ${TVS}/backend)
set(FE    ${TVS}/frontend)
set(TOOLS ${TVS}/tools)
set(GEN   ${CMAKE_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN})

# --- generated sources ---------------------------------------------------------
add_custom_command(OUTPUT ${GEN}/engine_struct.h
  COMMAND Python3::Interpreter ${TOOLS}/gen_struct.py ${BE}/engine.fields ${GEN}/engine_struct.h
  DEPENDS ${TOOLS}/gen_struct.py ${BE}/engine.fields)
add_custom_command(OUTPUT ${GEN}/fe_data.c ${GEN}/fe_data.h
  COMMAND Python3::Interpreter ${TOOLS}/fe_extract.py ${TV_DLL} -o ${GEN}
  DEPENDS ${TOOLS}/fe_extract.py ${TV_DLL})
add_custom_command(OUTPUT ${GEN}/tv_eng32.res
  COMMAND Python3::Interpreter ${TOOLS}/gen_res.py ${TV_DLL} ${GEN}/tv_eng32.res
  DEPENDS ${TOOLS}/gen_res.py ${TV_DLL})
add_custom_command(OUTPUT ${GEN}/res.o
  COMMAND ${WINDRES} -F pe-i386 -J res -O coff -i ${GEN}/tv_eng32.res -o ${GEN}/res.o
  DEPENDS ${GEN}/tv_eng32.res)
set_source_files_properties(${GEN}/res.o PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

set(TV_CFLAGS -std=gnu11 -O2 -g -Wall -Wno-unused-function -fwrapv -fno-strict-aliasing)
set(TV_DEFS __USE_MINGW_ANSI_STDIO=0)
set(TV_INCS ${BE} ${BE}/port ${GEN})

# --- the engine (sapi.c is CGRM_EN's SAPI layer, which TV_ENG32 does not have) --
file(GLOB ENGINE_SRCS ${BE}/engine/*.c)
list(REMOVE_ITEM ENGINE_SRCS ${BE}/engine/sapi.c)
add_library(tv_engine OBJECT ${ENGINE_SRCS}
  ${BE}/port/msvcrt.c ${BE}/port/stubs.c ${BE}/port/truvoice.c ${BE}/port/userdic.c
  ${GEN}/engine_struct.h)
target_compile_options(tv_engine PRIVATE ${TV_CFLAGS})
target_compile_definitions(tv_engine PRIVATE ${TV_DEFS})
target_include_directories(tv_engine PRIVATE ${TV_INCS})

# The engine's .rdata/.data, laid out as in the DLL, with a label at every
# symbol the engine objects reference.
add_custom_command(OUTPUT ${GEN}/tvdata.s
  COMMAND ${CMAKE_COMMAND} -E env NM=${CMAKE_NM}
          $<TARGET_FILE:Python3::Interpreter> ${TOOLS}/gen_data.py ${TV_DLL} ${BE} ${GEN}/tvdata.s
          $<TARGET_OBJECTS:tv_engine>
  DEPENDS tv_engine $<TARGET_OBJECTS:tv_engine> ${TOOLS}/gen_data.py ${TOOLS}/gen_hookmap.py
          ${TOOLS}/pe32.py ${TOOLS}/tvdata.py ${TV_DLL}
  COMMAND_EXPAND_LISTS)

# --- the email front end (all of it but the fediff bridges) -----------------------
file(GLOB FE_SRCS ${FE}/*.c)
list(FILTER FE_SRCS EXCLUDE REGEX "_bridge\\.c$")
add_library(tv_frontend OBJECT ${FE_SRCS} ${GEN}/fe_data.c ${BE}/port/email.c ${GEN}/engine_struct.h)
target_compile_options(tv_frontend PRIVATE ${TV_CFLAGS} -Wno-stringop-truncation -Wno-stringop-overflow)
target_compile_definitions(tv_frontend PRIVATE ${TV_DEFS})
target_include_directories(tv_frontend PRIVATE ${TV_INCS} ${FE})

# --- TV_ENG32.DLL ------------------------------------------------------------------
add_library(TV_ENG32 SHARED
  ${TVS}/dll/tts.c ${TVS}/dll/speak.c ${TVS}/dll/userdic.c ${TVS}/dll/tv_eng32.def
  ${GEN}/res.o ${GEN}/tvdata.s
  $<TARGET_OBJECTS:tv_engine> $<TARGET_OBJECTS:tv_frontend>)
set_target_properties(TV_ENG32 PROPERTIES PREFIX "" SUFFIX ".DLL")
target_compile_options(TV_ENG32 PRIVATE $<$<COMPILE_LANGUAGE:C>:${TV_CFLAGS}>)
target_compile_definitions(TV_ENG32 PRIVATE ${TV_DEFS})
target_include_directories(TV_ENG32 PRIVATE ${TV_INCS} ${FE})
target_link_options(TV_ENG32 PRIVATE -static-libgcc)
target_link_libraries(TV_ENG32 PRIVATE winmm comdlg32 gdi32 comctl32)
install(TARGETS TV_ENG32 RUNTIME DESTINATION . ARCHIVE EXCLUDE_FROM_ALL)
