cmake_minimum_required(VERSION 3.14)

if(WIN32)
    add_compile_options("/std:c++latest")
endif(WIN32)
set(CMAKE_CXX_STANDARD 17)

project(ShaderGL)

find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_library(ShaderGLLib
  STATIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Error.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Error.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Frame.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Frame.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Mesh.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Mesh.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Pixel.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Pixel.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Program.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Program.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Render.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Render.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Scene.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Scene.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.h
)

target_link_libraries(ShaderGLLib 
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(ShaderGLTest
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/BufferTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/BufferTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/CameraTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/DeviceTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/DeviceTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ErrorTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ErrorTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/FrameTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/FrameTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ImageTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/LightTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/LightTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/main.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MeshTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/PixelTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/PixelTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ProgramTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ProgramTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/RenderTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/RenderTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/SceneTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/SceneTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ShaderTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/ShaderTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/TextureTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/TextureTest.h
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/WindowTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/WindowTest.h
)

target_include_directories(ShaderGLTest
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(ShaderGLTest
  PUBLIC
    ShaderGLLib
  PRIVATE
    GTest::gtest
    GTest::gtest_main
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(JapaneseFlag
  WIN32
    ${PROJECT_SOURCE_DIR}/Asset/Shader/JapaneseFlag.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/JapaneseFlag.vert
    ${PROJECT_SOURCE_DIR}/Sample/JapaneseFlag/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/JapaneseFlag/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/JapaneseFlag/main.cpp
)

target_include_directories(JapaneseFlag
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(JapaneseFlag
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(RayMarching
  WIN32
    ${PROJECT_SOURCE_DIR}/Asset/Shader/RayMarching.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/RayMarching.vert
    ${PROJECT_SOURCE_DIR}/Sample/RayMarching/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/RayMarching/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/RayMarching/main.cpp
)

target_include_directories(RayMarching
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(RayMarching
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(Simple
  WIN32
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Simple.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Simple.vert
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/Simple/main.cpp
)

target_include_directories(Simple
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(Simple
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(PhysicallyBasedRendering
  WIN32
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Brightness.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Brightness.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Combine.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Combine.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMapHighDynamicRange.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMapHighDynamicRange.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Display.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Display.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/EquirectangularCubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/EquirectangularCubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/GaussianBlur.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/GaussianBlur.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IntegrateBRDF.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IntegrateBRDF.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IrradianceCubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IrradianceCubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/MonteCarloPrefilter.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/MonteCarloPrefilter.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/PhysicallyBasedRendering.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/PhysicallyBasedRendering.vert
    ${PROJECT_SOURCE_DIR}/Sample/PhysicallyBasedRendering/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/PhysicallyBasedRendering/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/PhysicallyBasedRendering/main.cpp
)

target_include_directories(PhysicallyBasedRendering
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(PhysicallyBasedRendering
  PUBLIC
    ShaderGLLib
  PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    glm
)
