cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

if(UNIX AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET x64-linux)
    endif()
    include("~/Documents/GitHub/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

project(Frame)

find_package(absl CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

if(WIN32)
    # Standard MSVC flags.
    add_compile_options("/std:c++latest")
    add_compile_options("/D _WIN32_WINNT=0x0A00")
    # This is there to link static in case there is a -static in the variable.
    if(${VCPKG_TARGET_TRIPLET} MATCHES "-static")
        string(REPLACE 
            "/MD"
            "/MT" 
            CMAKE_CXX_FLAGS 
            ${CMAKE_CXX_FLAGS})
        string(REPLACE 
            "/MD"
            "/MT" 
            CMAKE_CXX_FLAGS_DEBUG 
            ${CMAKE_CXX_FLAGS_DEBUG})
        string(REPLACE 
            "/MD"
            "/MT" 
            CMAKE_CXX_FLAGS_RELEASE 
            ${CMAKE_CXX_FLAGS_RELEASE})
        string(REPLACE 
            "/MD"
            "/MT" 
            CMAKE_CXX_FLAGS_MINSIZEREL 
            ${CMAKE_CXX_FLAGS_MINSIZEREL})
        string(REPLACE 
            "/MD"
            "/MT" 
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    endif(${VCPKG_TARGET_TRIPLET} MATCHES "-static")
endif(WIN32)

add_subdirectory(Asset/Shader/OpenGL)
add_subdirectory(Frame)
add_subdirectory(Sample)
