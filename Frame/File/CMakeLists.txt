# Frame Import.

add_library(FrameFile
  STATIC
	Image.cpp
	Image.h
    FileSystem.cpp
    FileSystem.h
)

target_include_directories(FrameFile
  PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameFile
  PUBLIC
	Frame
	FrameProto
	FrameOpenGL
	assimp::assimp
)

add_subdirectory(Test)
