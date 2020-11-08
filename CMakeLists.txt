cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project(ShaderGL)

find_package(absl CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")

if(WIN32)
    add_compile_options("/std:c++latest")
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

add_custom_target(ShaderGL
  SOURCES
    Asset/Shader/Blur.frag
    Asset/Shader/Blur.vert
    Asset/Shader/Brightness.frag
    Asset/Shader/Brightness.vert
    Asset/Shader/CubeMap.frag
    Asset/Shader/CubeMap.vert
    Asset/Shader/CubeMapDeferred.frag
    Asset/Shader/CubeMapDeferred.vert
    Asset/Shader/Display.frag
    Asset/Shader/Display.vert
    Asset/Shader/EquirectangularCubeMap.frag
    Asset/Shader/EquirectangularCubeMap.vert
    Asset/Shader/GaussianBlur.frag
    Asset/Shader/GaussianBlur.vert
    Asset/Shader/HighDynamicRange.frag
    Asset/Shader/HighDynamicRange.vert
    Asset/Shader/IntegrateBRDF.frag
    Asset/Shader/IntegrateBRDF.vert
    Asset/Shader/IrradianceCubeMap.frag
    Asset/Shader/IrradianceCubeMap.vert
    Asset/Shader/JapaneseFlag.frag
    Asset/Shader/JapaneseFlag.vert
    Asset/Shader/Lighting.frag
    Asset/Shader/Lighting.vert
    Asset/Shader/MonteCarloPrefilter.frag
    Asset/Shader/MonteCarloPrefilter.vert
    Asset/Shader/PhysicallyBasedRendering.frag
    Asset/Shader/PhysicallyBasedRendering.vert
    Asset/Shader/RayMarching.frag
    Asset/Shader/RayMarching.vert
    Asset/Shader/ScreenSpaceAmbientOcclusion.frag
    Asset/Shader/ScreenSpaceAmbientOcclusion.vert
    Asset/Shader/Simple.frag
    Asset/Shader/Simple.vert
    Asset/Shader/VectorAddition.frag
    Asset/Shader/VectorAddition.vert
    Asset/Shader/VectorMultiply.frag
    Asset/Shader/VectorMultiply.vert
    Asset/Shader/ViewPositionNormal.frag
    Asset/Shader/ViewPositionNormal.vert
)

add_library(FrameProto
  STATIC
    FrameProto/Display.proto
    FrameProto/Effect.proto
    FrameProto/Math.proto
    FrameProto/Pixel.proto
    FrameProto/Proto.h
    FrameProto/Size.proto
    FrameProto/Texture.proto
    FrameProto/Uniform.proto
)

target_include_directories(FrameProto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# I really don't know where the additional path are...
#  - APPEND_PATH is not?
#  - CMAKE_PREFIX_PATH doesn't work? 
#  - "${protobuf_DIR}/../../include/" <- this is the path to be added?
protobuf_generate(APPEND_PATH TARGET FrameProto)

target_link_libraries(FrameProto 
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf
    spdlog::spdlog
)

add_library(ShaderGLLib
  STATIC
    ShaderGLLib/Buffer.cpp
    ShaderGLLib/Buffer.h
    ShaderGLLib/Camera.cpp
    ShaderGLLib/Camera.h
    ShaderGLLib/Convert.cpp
    ShaderGLLib/Convert.h
    ShaderGLLib/Device.cpp
    ShaderGLLib/Device.h
    ShaderGLLib/Effect.cpp
    ShaderGLLib/Effect.h
    ShaderGLLib/Error.cpp
    ShaderGLLib/Error.h
    ShaderGLLib/Fill.cpp
    ShaderGLLib/Fill.h
    ShaderGLLib/Frame.cpp
    ShaderGLLib/Frame.h
    ShaderGLLib/Image.cpp
    ShaderGLLib/Image.h
    ShaderGLLib/Light.cpp
    ShaderGLLib/Light.h
    ShaderGLLib/Logger.cpp
    ShaderGLLib/Logger.h
    ShaderGLLib/Material.cpp
    ShaderGLLib/Material.h
    ShaderGLLib/Mesh.cpp
    ShaderGLLib/Mesh.h
    ShaderGLLib/Pixel.cpp
    ShaderGLLib/Pixel.h
    ShaderGLLib/Program.cpp
    ShaderGLLib/Program.h
    ShaderGLLib/Render.cpp
    ShaderGLLib/Render.h
    ShaderGLLib/Scene.cpp
    ShaderGLLib/Scene.h
    ShaderGLLib/ScopedBind.cpp
    ShaderGLLib/ScopedBind.h
    ShaderGLLib/Shader.cpp
    ShaderGLLib/Shader.h
    ShaderGLLib/Texture.cpp
    ShaderGLLib/Texture.h
    ShaderGLLib/Uniform.h
    ShaderGLLib/Window.cpp
    ShaderGLLib/Window.h
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
    spdlog::spdlog
)

add_executable(ShaderGLTest
    ShaderGLTest/BufferTest.cpp
    ShaderGLTest/BufferTest.h
    ShaderGLTest/CameraTest.cpp
    ShaderGLTest/CameraTest.h
    ShaderGLTest/ConvertTest.cpp
    ShaderGLTest/ConvertTest.h
    ShaderGLTest/DeviceTest.cpp
    ShaderGLTest/DeviceTest.h
    ShaderGLTest/EffectTest.cpp
    ShaderGLTest/EffectTest.h
    ShaderGLTest/ErrorTest.cpp
    ShaderGLTest/ErrorTest.h
    ShaderGLTest/FrameTest.cpp
    ShaderGLTest/FrameTest.h
    ShaderGLTest/ImageTest.cpp
    ShaderGLTest/ImageTest.h
    ShaderGLTest/LightTest.cpp
    ShaderGLTest/LightTest.h
    ShaderGLTest/main.cpp
    ShaderGLTest/MaterialTest.cpp
    ShaderGLTest/MaterialTest.h
    ShaderGLTest/MeshTest.cpp
    ShaderGLTest/MeshTest.h
    ShaderGLTest/PixelTest.cpp
    ShaderGLTest/PixelTest.h
    ShaderGLTest/ProgramMock.h
    ShaderGLTest/ProgramTest.cpp
    ShaderGLTest/ProgramTest.h
    ShaderGLTest/RenderTest.cpp
    ShaderGLTest/RenderTest.h
    ShaderGLTest/SceneTest.cpp
    ShaderGLTest/SceneTest.h
    ShaderGLTest/ShaderTest.cpp
    ShaderGLTest/ShaderTest.h
    ShaderGLTest/TextureTest.cpp
    ShaderGLTest/TextureTest.h
    ShaderGLTest/UniformMock.h
    ShaderGLTest/WindowTest.cpp
    ShaderGLTest/WindowTest.h
)

target_include_directories(ShaderGLTest
  PUBLIC
    ShaderGLLib
)

target_link_libraries(ShaderGLTest
  PUBLIC
    ShaderGLLib
    FrameProto
  PRIVATE
    protobuf::libprotoc
    protobuf::libprotobuf 
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
    SDL2::SDL2
    GLEW::GLEW
    glm
    spdlog::spdlog
)

add_executable(JapaneseFlag
  WIN32
    Sample/JapaneseFlag/Application.cpp
    Sample/JapaneseFlag/Application.h
    Sample/JapaneseFlag/main.cpp
)

target_include_directories(JapaneseFlag
  PUBLIC
    ShaderGLLib
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
    spdlog::spdlog
)

add_executable(RayMarching
  WIN32
    Sample/RayMarching/Application.cpp
    Sample/RayMarching/Application.h
    Sample/RayMarching/main.cpp
)

target_include_directories(RayMarching
  PUBLIC
    ShaderGLLib
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
    spdlog::spdlog
)

add_executable(Simple
  WIN32
    Sample/Simple/Application.cpp
    Sample/Simple/Application.h
    Sample/Simple/Draw.cpp
    Sample/Simple/Draw.h
    Sample/Simple/main.cpp
)

target_include_directories(Simple
  PUBLIC
    ShaderGLLib
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
    spdlog::spdlog
)

add_executable(SceneRendering
  WIN32
    Asset/SceneRendering.json
    Sample/SceneRendering/Application.cpp
    Sample/SceneRendering/Application.h
    Sample/SceneRendering/Draw.cpp
    Sample/SceneRendering/Draw.h
    Sample/SceneRendering/Input.cpp
    Sample/SceneRendering/Input.h
    Sample/SceneRendering/main.cpp
)

target_include_directories(SceneRendering
  PUBLIC
    ShaderGLLib
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
    spdlog::spdlog
)
