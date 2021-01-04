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

add_library(FrameProto
  STATIC
    Frame/Proto/ParseUniform.cpp
    Frame/Proto/ParseUniform.h
    Frame/Proto/ParseProgram.cpp
    Frame/Proto/ParseProgram.h
    Frame/Proto/LevelOpenGL.h
    Frame/Proto/Level.proto
    Frame/Proto/Material.proto
    Frame/Proto/Math.proto
    Frame/Proto/Pixel.proto
    Frame/Proto/Program.proto
    Frame/Proto/Proto.cpp
    Frame/Proto/Proto.h
    Frame/Proto/Scene.proto
    Frame/Proto/Size.proto
    Frame/Proto/Texture.proto
    Frame/Proto/Uniform.proto
)

target_include_directories(FrameProto 
  PUBLIC 
    ${CMAKE_SOURCE_DIR}
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Frame
  PUBLIC
    FrameProto
    SDL2::SDL2
    spdlog::spdlog
)

add_library(FrameOpenGL
  STATIC
    Frame/OpenGL/Buffer.cpp
    Frame/OpenGL/Buffer.h
    Frame/OpenGL/Camera.cpp
    Frame/OpenGL/Camera.h
    Frame/OpenGL/Device.cpp
    Frame/OpenGL/Device.h
    Frame/OpenGL/Fill.cpp
    Frame/OpenGL/Fill.h
    Frame/OpenGL/FrameBuffer.cpp
    Frame/OpenGL/FrameBuffer.h
    Frame/OpenGL/Image.cpp
    Frame/OpenGL/Image.h
    Frame/OpenGL/Light.cpp
    Frame/OpenGL/Light.h
    Frame/OpenGL/Material.cpp
    Frame/OpenGL/Material.h
    Frame/OpenGL/StaticMesh.cpp
    Frame/OpenGL/StaticMesh.h
    Frame/OpenGL/Pixel.cpp
    Frame/OpenGL/Pixel.h
    Frame/OpenGL/Program.cpp
    Frame/OpenGL/Program.h
    Frame/OpenGL/RenderBuffer.cpp
    Frame/OpenGL/RenderBuffer.h
    Frame/OpenGL/Rendering.cpp
    Frame/OpenGL/Rendering.h
    Frame/OpenGL/Scene.cpp
    Frame/OpenGL/Scene.h
    Frame/OpenGL/ScopedBind.cpp
    Frame/OpenGL/ScopedBind.h
    Frame/OpenGL/Shader.cpp
    Frame/OpenGL/Shader.h
    Frame/OpenGL/Texture.cpp
    Frame/OpenGL/Texture.h
)

target_include_directories(FrameOpenGL
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGL
  PUBLIC
    Frame
    FrameProto
    GLEW::GLEW
    glm
)

add_executable(FrameOpenGLTest
    Frame/OpenGL/Test/BufferTest.cpp
    Frame/OpenGL/Test/BufferTest.h
    Frame/OpenGL/Test/CameraTest.cpp
    Frame/OpenGL/Test/CameraTest.h
    Frame/OpenGL/Test/ConvertTest.cpp
    Frame/OpenGL/Test/ConvertTest.h
    Frame/OpenGL/Test/DeviceTest.cpp
    Frame/OpenGL/Test/DeviceTest.h
    Frame/OpenGL/Test/ErrorTest.cpp
    Frame/OpenGL/Test/ErrorTest.h
    Frame/OpenGL/Test/FrameBufferTest.cpp
    Frame/OpenGL/Test/FrameBufferTest.h
    Frame/OpenGL/Test/ImageTest.cpp
    Frame/OpenGL/Test/ImageTest.h
    Frame/OpenGL/Test/LightTest.cpp
    Frame/OpenGL/Test/LightTest.h
    Frame/OpenGL/Test/main.cpp
    Frame/OpenGL/Test/MaterialTest.cpp
    Frame/OpenGL/Test/MaterialTest.h
    Frame/OpenGL/Test/StaticMeshTest.cpp
    Frame/OpenGL/Test/StaticMeshTest.h
    Frame/OpenGL/Test/PixelTest.cpp
    Frame/OpenGL/Test/PixelTest.h
    Frame/OpenGL/Test/ProgramMock.h
    Frame/OpenGL/Test/ProgramTest.cpp
    Frame/OpenGL/Test/ProgramTest.h
    Frame/OpenGL/Test/RenderBufferTest.cpp
    Frame/OpenGL/Test/RenderBufferTest.h
    Frame/OpenGL/Test/SceneTest.cpp
    Frame/OpenGL/Test/SceneTest.h
    Frame/OpenGL/Test/ShaderTest.cpp
    Frame/OpenGL/Test/ShaderTest.h
    Frame/OpenGL/Test/TextureTest.cpp
    Frame/OpenGL/Test/TextureTest.h
    Frame/OpenGL/Test/UniformMock.h
    Frame/OpenGL/Test/WindowTest.cpp
    Frame/OpenGL/Test/WindowTest.h
)

target_include_directories(FrameOpenGLTest
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(FrameOpenGLTest
  PUBLIC
    FrameOpenGL
)

target_link_libraries(FrameOpenGLTest
  PUBLIC
    FrameOpenGL
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(JapaneseFlag
  PUBLIC
    Sample/JapaneseFlag
)

target_link_libraries(JapaneseFlag
  PUBLIC
    Frame
    FrameOpenGL
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(RayMarching
  PUBLIC
    Frame
    FrameOpenGL
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    Sample/SceneSimple
)

target_link_libraries(SceneSimple
  PUBLIC
    Frame
    FrameOpenGL
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
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    Sample/SceneRendering
)

target_link_libraries(SceneRendering
  PUBLIC
    Frame
    FrameOpenGL
    FrameProto
)
