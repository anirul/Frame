# Frame OpenGL File Test

add_executable(FrameOpenGLFileTest
  load_program_test.cpp
  load_program_test.h
  load_static_mesh_test.cpp
  load_static_mesh_test.h
  load_texture_test.cpp
  load_texture_test.h
  main.cpp
)

target_include_directories(FrameOpenGLFileTest
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../tests
  ${CMAKE_CURRENT_BINARY_DIR}
  FrameOpenGLFile
)

target_link_libraries(FrameOpenGLFileTest
  PUBLIC
  Frame
  FrameFile
  FrameOpenGL
  FrameOpenGLFile
  GTest::gmock
  GTest::gtest
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameOpenGLFileTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameOpenGLFileTest)

set_property(TARGET FrameOpenGLFileTest PROPERTY FOLDER "FrameTest/OpenGL")
