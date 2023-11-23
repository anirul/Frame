#Frame Delta Draw

add_library(FrameDeltaDraw
  STATIC
    device.cpp
    device.h
    sdl_deltadraw_none.cpp
	sdl_deltadraw_none.h
	sdl_deltadraw_window.cpp
	sdl_deltadraw_window.h
	window_factory.cpp
	window_factory.h
)

target_include_directories(FrameDeltaDraw
  PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/src/frame/proto
)

target_link_libraries(FrameDeltaDraw
  PUBLIC
	Frame
	FrameFile
	FrameJson
)

set_property(TARGET FrameDeltaDraw PROPERTY FOLDER "Frame/DeltaDraw")
