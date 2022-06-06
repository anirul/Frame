# Sample Common.

add_library(SampleCommon
  STATIC
	application.cpp
	application.h
	draw.cpp
	draw.h
	path_interface.h
)

target_include_directories(SampleCommon
  PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(SampleCommon
  PUBLIC
	Frame
	FrameProto
	FrameOpenGL
)

set_property(TARGET SampleCommon PROPERTY FOLDER "Sample")