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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -shared")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Global include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/lua
  ${CMAKE_CURRENT_SOURCE_DIR}/minhook/include
  ${WINDOWS_SYSTEM_INCLUDE_PATHS}
)

# Find all .c files in the lua directory, except for lua.c
file(GLOB LUA_SOURCES
  lua/*.c
  !lua/lua.c
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

# Enable solution generation for multiple Visual Studio versions
set(CMAKE_GENERATOR_PLATFORM x64)

# Add support for Visual Studio solution generation
option(VS_SOLUTION "Generate Visual Studio solution" ON)

if(VS_SOLUTION)
    # List of Visual Studio versions to generate solutions for
    set(VS_VERSIONS
        "Visual Studio 17 2022"  # VS 2022
        "Visual Studio 16 2019"  # VS 2019
        "Visual Studio 15 2017"  # VS 2017
    )

    foreach(VS_VERSION IN LISTS VS_VERSIONS)
        # Ensure the build directory exists
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/build_${VS_VERSION}")

        # Generate solution for each Visual Studio version
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -G "${VS_VERSION}"
                -A x64
                -S ${CMAKE_CURRENT_SOURCE_DIR}
                -B ${CMAKE_CURRENT_BINARY_DIR}/build_${VS_VERSION}
            RESULT_VARIABLE VS_GENERATION_RESULT
        )

        if(VS_GENERATION_RESULT EQUAL 0)
            message(STATUS "Generated solution for ${VS_VERSION}")
        else()
            message(WARNING "Failed to generate solution for ${VS_VERSION}")
        endif()
    endforeach()
endif()
