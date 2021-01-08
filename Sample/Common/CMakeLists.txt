# Sample Common.

add_library(SampleCommon
  STATIC
	Application.cpp
	Application.h
	Draw.cpp
	Draw.h
	main.cpp
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