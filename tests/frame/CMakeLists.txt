# Frame Test.

add_executable(FrameTest
	CameraTest.cpp
	CameraTest.h
	ProgramMock.h
	main.cpp
	UniformMock.h
	WindowTest.cpp
	WindowTest.h
)

target_include_directories(FrameTest
  PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameTest
  PUBLIC
	Frame
	GTest::gmock
	GTest::gtest
)

set_property(TARGET FrameTest PROPERTY FOLDER "Frame")