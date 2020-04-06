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
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.cpp
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
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Pixel.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Pixel.cpp
    ${PROJECT_SOURCE_DIR}/Asset/Simple.Vertex.glsl
    ${PROJECT_SOURCE_DIR}/Asset/Simple.Fragment.glsl
    ${PROJECT_SOURCE_DIR}/Asset/PBR.Vertex.glsl
    ${PROJECT_SOURCE_DIR}/Asset/PBR.Fragment.glsl
    ${PROJECT_SOURCE_DIR}/Asset/CubeMap.Vertex.glsl
    ${PROJECT_SOURCE_DIR}/Asset/CubeMap.Fragment.glsl
)

target_link_libraries(ShaderGLLib 
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
)

add_executable(ShaderGLTest
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/main.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/LightTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/LightTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/PixelTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/PixelTest.cpp
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
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/WindowTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/WindowTest.cpp
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
    ${PROJECT_SOURCE_DIR}/SimpleSample/Application.h
    ${PROJECT_SOURCE_DIR}/SimpleSample/Application.cpp
)

target_link_libraries(SimpleSample
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
)
