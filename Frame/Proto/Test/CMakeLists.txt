# Frame Proto Test.

add_executable(FrameProtoTest
	main.cpp
	ParseLevelTest.cpp
	ParseLevelTest.h
	ParseMaterialTest.cpp
	ParseMaterialTest.h
	ParsePixelTest.cpp
	ParsePixelTest.h
	ParseProgramTest.cpp
	ParseProgramTest.h
	ParseSceneTreeTest.cpp
	ParseSceneTreeTest.h
	ParseTextureTest.cpp
	ParseTextureTest.h
	ParseUniformTest.cpp
	ParseUniformTest.h
)

target_include_directories(FrameProtoTest
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    FrameProto
)

target_link_libraries(FrameProtoTest
  PUBLIC
    Frame
    FrameFile
    FrameOpenGL
    FrameOpenGLFile
    FrameProto
    GTest::gmock
    GTest::gtest
)

set_property(TARGET FrameProtoTest PROPERTY FOLDER "Frame")