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
  mesh_test.cpp
  mesh_test.h
  skinned_mesh_test.cpp
  skinned_mesh_test.h
  pixel_test.cpp
  pixel_test.h
  program_test.cpp
  program_test.h
  render_buffer_test.cpp
  render_buffer_test.h
  raytracing_test.cpp
  raytracing_test.h
  renderer_test.cpp
  renderer_test.h
  shader_test.cpp
  shader_test.h
  window_glsl_file_test.cpp
  window_glsl_file_test.h
  cubemap_test.cpp
  cubemap_test.h
  texture_test.cpp
  texture_test.h
  window_test.cpp
  window_test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/device_test.json
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../asset/json/renderer_test.json
)

frame_enable_internal_backend_access(FrameOpenGLTest)

target_include_directories(FrameOpenGLTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameOpenGLTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_LINK_GROUP_END}
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameOpenGLTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameOpenGLTest TEST_LIST FRAME_OPENGL_TESTS)
set_tests_properties(${FRAME_OPENGL_TESTS}
  PROPERTIES LABELS requires-opengl)

add_subdirectory(file)

set_property(TARGET FrameOpenGLTest PROPERTY FOLDER "FrameTest/OpenGL")

add_executable(FrameOpenGLGUITest
  gui_test.cpp
)

frame_enable_internal_backend_access(FrameOpenGLGUITest)

target_include_directories(FrameOpenGLGUITest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_BINARY_DIR}/external/Frame/frame/proto/generated
)

target_link_libraries(FrameOpenGLGUITest
  PUBLIC
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
)

if(TARGET fmt::fmt)
  target_link_libraries(FrameOpenGLGUITest PUBLIC fmt::fmt)
endif()

set_target_properties(FrameOpenGLGUITest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

if(MSVC)
  target_compile_options(FrameOpenGLGUITest PRIVATE /utf-8)
endif()

gtest_add_tests(TARGET FrameOpenGLGUITest TEST_LIST FRAME_OPENGL_GUI_TESTS)
set_tests_properties(${FRAME_OPENGL_GUI_TESTS}
  PROPERTIES LABELS requires-opengl)

set_property(TARGET FrameOpenGLGUITest PROPERTY FOLDER "FrameTest/OpenGL")
