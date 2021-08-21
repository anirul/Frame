# Frame Import.

add_library(FrameFile
  STATIC
	FileSystem.cpp
    FileSystem.h
	Image.cpp
	Image.h
    Obj.cpp
	Obj.h
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
	tinyobjloader::tinyobjloader
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")

add_subdirectory(Test)
