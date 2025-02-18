cmake_minimum_required(VERSION 3.12)
project(vscodelua C CXX)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Set cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX "x86_64-w64-mingw32")

# Set up cross-compilation toolchain
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}-g++")
set(CMAKE_RC_COMPILER "${TOOLCHAIN_PREFIX}-windres")
set(CMAKE_SYSTEM_LIBRARY_PATH "/usr/${TOOLCHAIN_PREFIX}/lib")

# Windows system include paths
set(WINDOWS_SYSTEM_INCLUDE_PATHS
  "/usr/${TOOLCHAIN_PREFIX}/include"
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -shared")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Global include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/cryengine/Code/SDKs/Lua/src
  ${CMAKE_CURRENT_SOURCE_DIR}/minhook/include
  ${WINDOWS_SYSTEM_INCLUDE_PATHS}
)

# Find all .c files in the lua directory, except for the interpreter
file(GLOB LUA_SOURCES
cryengine/Code/SDKs/Lua/src/*.c
  !cryengine/Code/SDKs/Lua/src/lua.c
  !cryengine/Code/SDKs/Lua/src/luac.c
  !cryengine/Code/SDKs/Lua/src/print.c
)

# Add Lua
add_library(lua STATIC
  ${LUA_SOURCES}
)

# Add MinHook
add_library(minhook STATIC
  minhook/src/buffer.c
  minhook/src/hook.c
  minhook/src/trampoline.c
  minhook/src/hde/hde64.c
)

# Main mod DLL
add_library(vscodelua SHARED
  src/dllmain.cpp
)

# Link libraries
target_link_libraries(vscodelua
  lua
  minhook
  ws2_32
  -static # so we don't need to mess around with winetricks or protontricks
)

# Set output name and properties
set_target_properties(vscodelua PROPERTIES
  PREFIX ""
  OUTPUT_NAME "vscodelua"
  SUFFIX ".asi"
)
