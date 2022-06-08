# Frame Import.

add_library(FrameFile
  STATIC
  	${CMAKE_SOURCE_DIR}/include/frame/file/file_system.h
	file_system.cpp
	image.cpp
	image.h
    obj.cpp
	obj.h
)

target_include_directories(FrameFile
  PUBLIC
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameFile
  PUBLIC
	Frame
	FrameJson
	FrameOpenGL
	tinyobjloader
	stb::stb
)

set_property(TARGET FrameFile PROPERTY FOLDER "Frame")
