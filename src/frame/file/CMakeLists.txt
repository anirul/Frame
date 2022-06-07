# Frame Import.

add_library(FrameFile
  STATIC
	file_system.cpp
    file_system.h
	image.cpp
	image.h
    obj.cpp
	obj.h
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
	tinyobjloader
	stb::stb
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")
