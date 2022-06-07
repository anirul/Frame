# Frame File Test.

add_executable(FrameFileTest
	file_system_test.cpp
	file_system_test.h
	image_test.cpp
	image_test.h
	main.cpp
	obj_test.cpp
	obj_test.h
)

target_include_directories(FrameFileTest
  PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${STB_INCLUDE_DIRS}
)

target_link_libraries(FrameFileTest
  PUBLIC
	Frame
	FrameFile
	GTest::gmock
	GTest::gtest
)

set_property(TARGET FrameFileTest PROPERTY FOLDER "Frame")
