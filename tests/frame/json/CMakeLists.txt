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
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/scene_tree_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/raytracing.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/dragon.json
)

target_include_directories(FrameJsonTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameJsonTest
  PUBLIC
  GTest::gmock
  GTest::gtest
  ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
  ${FRAME_LINK_GROUP_END}
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameJsonTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameJsonTest TEST_LIST FRAME_JSON_TESTS)
set(FRAME_JSON_OPENGL_TESTS
  ParseMaterialTest.CreateParseMaterialTest
  ParseProgramTest.CreateParseProgramTest
  ParseProgramTest.CreateParseProgramUniformTest
  ParseTextureTest.CreateInvalidTextureFromProtoTest
  ParseTextureTest.CreateTextureFromProtoCheckSizeTest
  ParseTextureTest.CreateTextureFromProtoWrongSizeTest)
set_tests_properties(${FRAME_JSON_OPENGL_TESTS}
  PROPERTIES LABELS requires-opengl)

set_property(TARGET FrameJsonTest PROPERTY FOLDER "FrameTest")
