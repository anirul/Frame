# Sample Common.

add_library(LibCommon
  STATIC
	application.cpp
	application.h
	draw.cpp
	draw.h
	path_interface.h
)

target_include_directories(LibCommon
  PUBLIC
	${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(LibCommon
  PUBLIC
	Frame
	FrameProto
	FrameOpenGL
)

set_property(TARGET LibCommon PROPERTY FOLDER "Examples")
