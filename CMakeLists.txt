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
    Asset/Shader/SceneSimple.frag
    Asset/Shader/SceneSimple.vert
    Asset/Shader/VectorAddition.frag
    Asset/Shader/VectorAddition.vert
    Asset/Shader/VectorMultiply.frag
    Asset/Shader/VectorMultiply.vert
    Asset/Shader/ViewPositionNormal.frag
    Asset/Shader/ViewPositionNormal.vert
)

add_library(FrameProto
  STATIC
    FrameProto/Effect.proto
    FrameProto/Level.proto
    FrameProto/Math.proto
    FrameProto/Pixel.proto
    FrameProto/Proto.h
    FrameProto/Scene.proto
    FrameProto/Size.proto
    FrameProto/Texture.proto
    FrameProto/Uniform.proto
)

target_include_directories(FrameProto 
  PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}
)

# I still doesn't know how to pass the path correctly to the generate in case
# there is any external include.
protobuf_generate(APPEND_PATH TARGET FrameProto)

target_link_libraries(FrameProto 
  PUBLIC
    protobuf::libprotoc
    protobuf::libprotobuf
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
    ShaderGLLib/FrameBuffer.cpp
    ShaderGLLib/FrameBuffer.h
    ShaderGLLib/Image.cpp
    ShaderGLLib/Image.h
    ShaderGLLib/Light.cpp
    ShaderGLLib/Light.h
    ShaderGLLib/Logger.cpp
    ShaderGLLib/Logger.h
    ShaderGLLib/Material.cpp
    ShaderGLLib/Material.h
    ShaderGLLib/StaticMesh.cpp
    ShaderGLLib/StaticMesh.h
    ShaderGLLib/Pixel.cpp
    ShaderGLLib/Pixel.h
    ShaderGLLib/Program.cpp
    ShaderGLLib/Program.h
    ShaderGLLib/RenderBuffer.cpp
    ShaderGLLib/RenderBuffer.h
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
    ShaderGLTest/FrameBufferTest.cpp
    ShaderGLTest/FrameBufferTest.h
    ShaderGLTest/ImageTest.cpp
    ShaderGLTest/ImageTest.h
    ShaderGLTest/LightTest.cpp
    ShaderGLTest/LightTest.h
    ShaderGLTest/main.cpp
    ShaderGLTest/MaterialTest.cpp
    ShaderGLTest/MaterialTest.h
    ShaderGLTest/StaticMeshTest.cpp
    ShaderGLTest/StaticMeshTest.h
    ShaderGLTest/PixelTest.cpp
    ShaderGLTest/PixelTest.h
    ShaderGLTest/ProgramMock.h
    ShaderGLTest/ProgramTest.cpp
    ShaderGLTest/ProgramTest.h
    ShaderGLTest/RenderBufferTest.cpp
    ShaderGLTest/RenderBufferTest.h
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
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
)

add_executable(JapaneseFlag
  WIN32
    Asset/Json/JapaneseFLag.Effect.json
    Asset/Json/JapaneseFlag.Level.json
    Asset/Json/JapaneseFlag.Scene.json
    Asset/Json/JapaneseFLag.Texture.json
    Sample/Common/Application.cpp
    Sample/Common/Application.h
    Sample/Common/main.cpp
    Sample/Common/Draw.cpp
    Sample/Common/Draw.h
    Sample/JapaneseFlag/Name.cpp
    Sample/JapaneseFlag/Name.h
)

target_include_directories(JapaneseFlag
  PUBLIC
    ShaderGLLib
    Sample/JapaneseFlag
)

target_link_libraries(JapaneseFlag
  PUBLIC
    ShaderGLLib
    FrameProto
)

add_executable(RayMarching
  WIN32
    Asset/Json/RayMarching.Effect.json
    Asset/Json/RayMarching.Level.json
    Asset/Json/RayMarching.Scene.json
    Asset/Json/RayMarching.Texture.json
    Sample/Common/Application.cpp
    Sample/Common/Application.h
    Sample/Common/main.cpp
    Sample/Common/Draw.cpp
    Sample/Common/Draw.h
    Sample/RayMarching/Name.cpp
    Sample/rayMarching/Name.h
)

target_include_directories(RayMarching
  PUBLIC
    ShaderGLLib
    Sample/RayMarching
)

target_link_libraries(RayMarching
  PUBLIC
    ShaderGLLib
    FrameProto
)

add_executable(SceneSimple
  WIN32
    Asset/Json/SceneSimple.Effect.json
    Asset/Json/SceneSimple.Level.json
    Asset/Json/SceneSimple.Scene.json
    Asset/Json/SceneSimple.Texture.json
    Sample/Common/Application.cpp
    Sample/Common/Application.h
    Sample/Common/main.cpp
    Sample/Common/Draw.cpp
    Sample/Common/Draw.h
    Sample/SceneSimple/Name.cpp
    Sample/SceneSimple/Name.h
)

target_include_directories(SceneSimple
  PUBLIC
    ShaderGLLib
    Sample/SceneSimple
)

target_link_libraries(SceneSimple
  PUBLIC
    ShaderGLLib
    FrameProto
)

add_executable(SceneRendering
  WIN32
    Asset/Json/SceneRendering.Effect.json
    Asset/Json/SceneRendering.Level.json
    Asset/Json/SceneRendering.Scene.json
    Asset/Json/SceneRendering.Texture.json
    Sample/SceneRendering/main.cpp
    Sample/SceneRendering/Application.cpp
    Sample/SceneRendering/Application.h
    Sample/SceneRendering/Draw.cpp
    Sample/SceneRendering/Draw.h
    Sample/SceneRendering/Input.cpp
    Sample/SceneRendering/Input.h
)

target_include_directories(SceneRendering
  PUBLIC
    ShaderGLLib
    Sample/SceneRendering
)

target_link_libraries(SceneRendering
  PUBLIC
    ShaderGLLib
    FrameProto
)
