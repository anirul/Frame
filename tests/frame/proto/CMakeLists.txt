# Frame Proto Test.

add_executable(FrameProtoTest
	main.cpp
	parse_level_test.cpp
	parse_level_test.h
	parse_material_test.cpp
	parse_material_test.h
	parse_pixel_test.cpp
	parse_pixel_test.h
	parse_program_test.cpp
	parse_program_test.h
	parse_scene_tree_test.cpp
	parse_scene_tree_test.h
	parse_texture_test.cpp
	parse_texture_test.h
	parse_uniform_test.cpp
	parse_uniform_test.h
	${CMAKE_SOURCE_DIR}/asset/json/level_test.json
	${CMAKE_SOURCE_DIR}/asset/json/material_test.json
	${CMAKE_SOURCE_DIR}/asset/json/program_test.json
	${CMAKE_SOURCE_DIR}/asset/json/scene_tree_test.json
)

target_include_directories(FrameProtoTest
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameProtoTest
  PUBLIC
  	Frame
  	FrameFile
  	FrameProto
  	GTest::gmock
  	GTest::gtest
)

set_property(TARGET FrameProtoTest PROPERTY FOLDER "Frame")
