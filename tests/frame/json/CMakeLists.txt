# Frame Json Test.

add_executable(FrameJsonTest
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
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/level_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/material_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/program_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/scene_tree_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/raytracing.json
)

target_include_directories(FrameJsonTest
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameJsonTest
  PUBLIC
  Frame
  FrameFile
  FrameJson
  FrameProto
  GTest::gmock
  GTest::gtest
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameJsonTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_discover_tests(FrameJsonTest)

set_property(TARGET FrameJsonTest PROPERTY FOLDER "FrameTest")
