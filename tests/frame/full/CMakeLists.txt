# Frame Full Test.

add_executable(FrameFullTest
  main.cpp
  scene_simple_test.cpp
  scene_simple_test.h
  image_based_lighting_test.cpp
  image_based_lighting_test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/scene_simple.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/image_based_lighting.json
)

target_include_directories(FrameFullTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameFullTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    Frame
    FrameFile
    FrameJson
    FrameOpenGL
    FrameOpenGLFile
)

set_target_properties(FrameFullTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameFullTest)

set_property(TARGET FrameFullTest PROPERTY FOLDER "FrameTest/Full")
