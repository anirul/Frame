# Frame OpenGL Test.

add_executable(FrameOpenGLTest
  buffer_test.cpp
  buffer_test.h
  device_test.cpp
  device_test.h
  frame_buffer_test.cpp
  frame_buffer_test.h
  light_test.cpp
  light_test.h
  main.cpp
  material_test.cpp
  material_test.h
  static_mesh_test.cpp
  static_mesh_test.h
  pixel_test.cpp
  pixel_test.h
  program_test.cpp
  program_test.h
  render_buffer_test.cpp
  render_buffer_test.h
  renderer_test.cpp
  renderer_test.h
  shader_test.cpp
  shader_test.h
  cubemap_test.cpp
  cubemap_test.h
  texture_test.cpp
  texture_test.h
  image_based_lighting_test.cpp
  image_based_lighting_test.h
  scene_simple_test.cpp
  scene_simple_test.h
  window_test.cpp
  window_test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/device_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/renderer_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/scene_simple.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/image_based_lighting.json
)

target_include_directories(FrameOpenGLTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGLTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    FrameFile
    FrameOpenGL
    FrameOpenGLFile
    FrameJson
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameOpenGLTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameOpenGLTest)

add_subdirectory(file)

set_property(TARGET FrameOpenGLTest PROPERTY FOLDER "FrameTest/OpenGL")
