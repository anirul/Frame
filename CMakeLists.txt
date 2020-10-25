cmake_minimum_required(VERSION 3.14)

if(WIN32)
    add_compile_options("/std:c++latest")
endif(WIN32)
set(CMAKE_CXX_STANDARD 17)

project(ShaderGL)

find_package(absl CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_custom_target(ShaderGL
  SOURCES
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Blur.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Blur.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Brightness.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Brightness.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMapDeferred.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/CubeMapDeferred.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Display.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Display.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/EquirectangularCubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/EquirectangularCubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/GaussianBlur.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/GaussianBlur.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/HighDynamicRange.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/HighDynamicRange.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IntegrateBRDF.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IntegrateBRDF.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IrradianceCubeMap.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/IrradianceCubeMap.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/JapaneseFlag.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/JapaneseFlag.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Lighting.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Lighting.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/MonteCarloPrefilter.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/MonteCarloPrefilter.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/PhysicallyBasedRendering.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/PhysicallyBasedRendering.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/RayMarching.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/RayMarching.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/ScreenSpaceAmbientOcclusion.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/ScreenSpaceAmbientOcclusion.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Simple.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/Simple.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/VectorAddition.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/VectorAddition.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/VectorMultiply.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/VectorMultiply.vert
    ${PROJECT_SOURCE_DIR}/Asset/Shader/ViewPositionNormal.frag
    ${PROJECT_SOURCE_DIR}/Asset/Shader/ViewPositionNormal.vert
)

add_library(FrameProto
  STATIC
    ${PROJECT_SOURCE_DIR}/Asset/SceneRendering.json
    ${PROJECT_SOURCE_DIR}/FrameProto/Display.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Display.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Display.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Effect.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Effect.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Effect.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Math.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Math.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Math.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Pixel.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Pixel.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Pixel.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Proto.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Size.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Size.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Size.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Texture.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Texture.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Texture.proto
    ${PROJECT_SOURCE_DIR}/FrameProto/Uniform.pb.cc
    ${PROJECT_SOURCE_DIR}/FrameProto/Uniform.pb.h
    ${PROJECT_SOURCE_DIR}/FrameProto/Uniform.proto
)

target_link_libraries(FrameProto 
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
)

add_library(ShaderGLLib
  STATIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Buffer.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Camera.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Convert.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Convert.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Device.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Effect.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Effect.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Error.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Error.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Fill.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Fill.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Frame.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Frame.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Image.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Light.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Material.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Material.h
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
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/ScopedBind.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/ScopedBind.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Shader.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Texture.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Uniform.h
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLLib/Window.h
)

target_link_libraries(ShaderGLLib
  PUBLIC
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
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
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MaterialTest.cpp
    ${PROJECT_SOURCE_DIR}/ShaderGLTest/MaterialTest.h
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
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    GTest::gtest
    GTest::gtest_main
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(JapaneseFlag
  WIN32
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
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(RayMarching
  WIN32
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
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(Simple
  WIN32
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Draw.cpp
    ${PROJECT_SOURCE_DIR}/Sample/Simple/Draw.h
    ${PROJECT_SOURCE_DIR}/Sample/Simple/main.cpp
)

target_include_directories(Simple
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(Simple
  PUBLIC
    ShaderGLLib
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    SDL2::SDL2
    GLEW::GLEW
    glm
)

add_executable(SceneRendering
  WIN32
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Application.cpp
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Application.h
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Draw.cpp
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Draw.h
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Input.cpp
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/Input.h
    ${PROJECT_SOURCE_DIR}/Sample/SceneRendering/main.cpp
)

target_include_directories(SceneRendering
  PUBLIC
    ${PROJECT_SOURCE_DIR}/ShaderGLLib
)

target_link_libraries(SceneRendering
  PUBLIC
    ShaderGLLib
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    SDL2::SDL2
    GLEW::GLEW
    glm
)
