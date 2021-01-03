cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

project(Frame)

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

add_custom_target(OpenGLShader
  SOURCES
    Asset/Shader/OpenGL/Blur.frag
    Asset/Shader/OpenGL/Blur.vert
    Asset/Shader/OpenGL/Brightness.frag
    Asset/Shader/OpenGL/Brightness.vert
    Asset/Shader/OpenGL/CubeMap.frag
    Asset/Shader/OpenGL/CubeMap.vert
    Asset/Shader/OpenGL/CubeMapDeferred.frag
    Asset/Shader/OpenGL/CubeMapDeferred.vert
    Asset/Shader/OpenGL/Display.frag
    Asset/Shader/OpenGL/Display.vert
    Asset/Shader/OpenGL/EquirectangularCubeMap.frag
    Asset/Shader/OpenGL/EquirectangularCubeMap.vert
    Asset/Shader/OpenGL/GaussianBlur.frag
    Asset/Shader/OpenGL/GaussianBlur.vert
    Asset/Shader/OpenGL/HighDynamicRange.frag
    Asset/Shader/OpenGL/HighDynamicRange.vert
    Asset/Shader/OpenGL/IntegrateBRDF.frag
    Asset/Shader/OpenGL/IntegrateBRDF.vert
    Asset/Shader/OpenGL/IrradianceCubeMap.frag
    Asset/Shader/OpenGL/IrradianceCubeMap.vert
    Asset/Shader/OpenGL/JapaneseFlag.frag
    Asset/Shader/OpenGL/JapaneseFlag.vert
    Asset/Shader/OpenGL/Lighting.frag
    Asset/Shader/OpenGL/Lighting.vert
    Asset/Shader/OpenGL/MonteCarloPrefilter.frag
    Asset/Shader/OpenGL/MonteCarloPrefilter.vert
    Asset/Shader/OpenGL/PhysicallyBasedRendering.frag
    Asset/Shader/OpenGL/PhysicallyBasedRendering.vert
    Asset/Shader/OpenGL/RayMarching.frag
    Asset/Shader/OpenGL/RayMarching.vert
    Asset/Shader/OpenGL/ScreenSpaceAmbientOcclusion.frag
    Asset/Shader/OpenGL/ScreenSpaceAmbientOcclusion.vert
    Asset/Shader/OpenGL/SceneSimple.frag
    Asset/Shader/OpenGL/SceneSimple.vert
    Asset/Shader/OpenGL/VectorAddition.frag
    Asset/Shader/OpenGL/VectorAddition.vert
    Asset/Shader/OpenGL/VectorMultiply.frag
    Asset/Shader/OpenGL/VectorMultiply.vert
    Asset/Shader/OpenGL/ViewPositionNormal.frag
    Asset/Shader/OpenGL/ViewPositionNormal.vert
)

add_library(Frame
  STATIC
    Frame/BindInterface.h
    Frame/BufferInterface.h
    Frame/CameraInterface.h
    Frame/DeviceInterface.h
    Frame/DrawInterface.h
    Frame/Error.cpp
    Frame/Error.h
    Frame/Frame.h
    Frame/InputInterface.h
    Frame/LevelBase.h
    Frame/LevelInterface.h
    Frame/LightInterface.h
    Frame/Logger.cpp
    Frame/Logger.h
    Frame/MaterialInterface.h
    Frame/ProgramInterface.h
    Frame/SceneNodeInterface.h
    Frame/SceneTreeInterface.h
    Frame/StaticMeshInterface.h
    Frame/TextureInterface.h
    Frame/UniformInterface.h
    Frame/Window.cpp
    Frame/Window.h
    Frame/WindowInterface.h
)

target_include_directories(Frame
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Frame
  PUBLIC
    SDL2::SDL2
    spdlog::spdlog
)

add_library(FrameProto
  STATIC
    FrameProto/Convert.cpp
    FrameProto/Convert.h
    FrameProto/LevelOpenGL.h
    FrameProto/Level.proto
    FrameProto/Material.proto
    FrameProto/Math.proto
    FrameProto/Pixel.proto
    FrameProto/Program.proto
    FrameProto/Proto.cpp
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

add_library(OpenGLLib
  STATIC
    OpenGLLib/Buffer.cpp
    OpenGLLib/Buffer.h
    OpenGLLib/Camera.cpp
    OpenGLLib/Camera.h
    OpenGLLib/Device.cpp
    OpenGLLib/Device.h
    OpenGLLib/Fill.cpp
    OpenGLLib/Fill.h
    OpenGLLib/FrameBuffer.cpp
    OpenGLLib/FrameBuffer.h
    OpenGLLib/Image.cpp
    OpenGLLib/Image.h
    OpenGLLib/Light.cpp
    OpenGLLib/Light.h
    OpenGLLib/Material.cpp
    OpenGLLib/Material.h
    OpenGLLib/StaticMesh.cpp
    OpenGLLib/StaticMesh.h
    OpenGLLib/Pixel.cpp
    OpenGLLib/Pixel.h
    OpenGLLib/Program.cpp
    OpenGLLib/Program.h
    OpenGLLib/RenderBuffer.cpp
    OpenGLLib/RenderBuffer.h
    OpenGLLib/Rendering.cpp
    OpenGLLib/Rendering.h
    OpenGLLib/Scene.cpp
    OpenGLLib/Scene.h
    OpenGLLib/ScopedBind.cpp
    OpenGLLib/ScopedBind.h
    OpenGLLib/Shader.cpp
    OpenGLLib/Shader.h
    OpenGLLib/Texture.cpp
    OpenGLLib/Texture.h
)

target_link_libraries(OpenGLLib
  PUBLIC
    Frame
    FrameProto
    GLEW::GLEW
    glm
)

add_executable(OpenGLTest
    OpenGLTest/BufferTest.cpp
    OpenGLTest/BufferTest.h
    OpenGLTest/CameraTest.cpp
    OpenGLTest/CameraTest.h
    OpenGLTest/ConvertTest.cpp
    OpenGLTest/ConvertTest.h
    OpenGLTest/DeviceTest.cpp
    OpenGLTest/DeviceTest.h
    OpenGLTest/ErrorTest.cpp
    OpenGLTest/ErrorTest.h
    OpenGLTest/FrameBufferTest.cpp
    OpenGLTest/FrameBufferTest.h
    OpenGLTest/ImageTest.cpp
    OpenGLTest/ImageTest.h
    OpenGLTest/LightTest.cpp
    OpenGLTest/LightTest.h
    OpenGLTest/main.cpp
    OpenGLTest/MaterialTest.cpp
    OpenGLTest/MaterialTest.h
    OpenGLTest/StaticMeshTest.cpp
    OpenGLTest/StaticMeshTest.h
    OpenGLTest/PixelTest.cpp
    OpenGLTest/PixelTest.h
    OpenGLTest/ProgramMock.h
    OpenGLTest/ProgramTest.cpp
    OpenGLTest/ProgramTest.h
    OpenGLTest/RenderBufferTest.cpp
    OpenGLTest/RenderBufferTest.h
    OpenGLTest/SceneTest.cpp
    OpenGLTest/SceneTest.h
    OpenGLTest/ShaderTest.cpp
    OpenGLTest/ShaderTest.h
    OpenGLTest/TextureTest.cpp
    OpenGLTest/TextureTest.h
    OpenGLTest/UniformMock.h
    OpenGLTest/WindowTest.cpp
    OpenGLTest/WindowTest.h
)

target_include_directories(OpenGLTest
  PUBLIC
    ShaderGLLib
)

target_link_libraries(OpenGLTest
  PUBLIC
    ShaderGLLib
    FrameProto
    GTest::gmock
    GTest::gtest
    GTest::gtest_main
)

add_executable(JapaneseFlag
  WIN32
    Asset/Json/JapaneseFLag.Program.json
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
    Sample/JapaneseFlag
)

target_link_libraries(JapaneseFlag
  PUBLIC
    Frame
    OpenGLLib
    FrameProto
)

add_executable(RayMarching
  WIN32
    Asset/Json/RayMarching.Program.json
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
    Sample/RayMarching
)

target_link_libraries(RayMarching
  PUBLIC
    Frame
    OpenGLLib
    FrameProto
)

add_executable(SceneSimple
  WIN32
    Asset/Json/SceneSimple.Program.json
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
    Sample/SceneSimple
)

target_link_libraries(SceneSimple
  PUBLIC
    Frame
    OpenGLLib
    FrameProto
)

add_executable(SceneRendering
  WIN32
    Asset/Json/SceneRendering.Program.json
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
    OpenGLLib
    Sample/SceneRendering
)

target_link_libraries(SceneRendering
  PUBLIC
    OpenGLLib
    FrameProto
)
