# Frame File Test.

add_executable(FrameFileTest
	FileSystemTest.cpp
	FileSystemTest.h
	ImageTest.cpp
	ImageTest.h
	main.cpp
	ObjTest.cpp
	ObjTest.h
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
