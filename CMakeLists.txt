cmake_minimum_required(VERSION 3.14)

if(WIN32)
    add_compile_options("/std:c++latest")
endif(WIN32)
set(CMAKE_CXX_STANDARD 17)

project(ShaderGL)

find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Gtest CONFIG REQUIRED)

add_library(ShaderGLLib
  STATIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Mesh.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Mesh.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Vector.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Vector.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Scene.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Scene.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Program.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Program.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.cpp
    ${PROJECT_SOURCE_DIR}/Asset/SimpleVertex.glsl
    ${PROJECT_SOURCE_DIR}/Asset/SimpleFragment.glsl
    ${PROJECT_SOURCE_DIR}/Asset/PBRVertex.glsl
    ${PROJECT_SOURCE_DIR}/Asset/PBRFragment.glsl
)

target_link_libraries(ShaderGLLib 
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
)

add_executable(ShaderGLTest
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/main.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/OpenGLTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/VectorTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/VectorTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/SceneTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/SceneTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ShaderTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ShaderTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ProgramTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ProgramTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/TextureTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/TextureTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/DeviceTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/DeviceTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/BufferTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/BufferTest.cpp
)

target_link_libraries(ShaderGLTest
  PUBLIC
    ShaderGLLib
  PRIVATE
    GTest::gtest
    GTest::gtest_main
    SDL2::SDL2
    GLEW::GLEW
)

add_executable(SimpleSample
  WIN32
    ${PROJECT_SOURCE_DIR}/SimpleSample/main.cpp
    ${PROJECT_SOURCE_DIR}/SimpleSample/WindowInterface.h
    ${PROJECT_SOURCE_DIR}/SimpleSample/WindowSoftwareGL.h
    ${PROJECT_SOURCE_DIR}/SimpleSample/WindowSoftwareGL.cpp
    ${PROJECT_SOURCE_DIR}/SimpleSample/WindowSDL2GL.cpp
    ${PROJECT_SOURCE_DIR}/SimpleSample/WindowSDL2GL.h
)

target_link_libraries(SimpleSample
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
)
