# Frame Test.

add_executable(FrameTest
	CameraTest.cpp
	CameraTest.h
	ErrorTest.cpp
	ErrorTest.h
	main.cpp
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
	GTest::gtest_main
)