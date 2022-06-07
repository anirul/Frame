# Frame Test.

add_executable(FrameTest
	camera_test.cpp
	camera_test.h
	main.cpp
	program_mock.h
	uniform_mock.h
	window_test.cpp
	window_test.h
)

target_include_directories(FrameTest
  PUBLIC
	${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameTest
  PUBLIC
	Frame
	GTest::gmock
	GTest::gtest
)

set_property(TARGET FrameTest PROPERTY FOLDER "Frame")

add_subdirectory(file)
add_subdirectory(open_gl)
add_subdirectory(proto)
