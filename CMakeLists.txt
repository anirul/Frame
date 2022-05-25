cmake_minimum_required(VERSION 3.20)

project(Frame)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#    include("~/Documents/GitHub/vcpkg/scripts/buildsystems/vcpkg.cmake")
#endif()
#if(UNIX AND NOT DEFINED VCPKG_TARGET_TRIPLET)
#    set(VCPKG_TARGET_TRIPLET x64-linux)
#endif()
#if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
#    set(VCPKG_TARGET_TRIPLET x64-windows)
#endif()
if(WIN32)
    # Standard MSVC flags.
    add_compile_options("/std:c++20")
    add_compile_options("/D _WIN32_WINNT=0x0A00")
endif(WIN32)

# Conan install. 
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
# conan_basic_setup()

find_package(glm CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(glew REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

add_subdirectory(Asset/Shader/OpenGL)
add_subdirectory(Frame)
add_subdirectory(Sample)