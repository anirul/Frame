# Frame Test.

add_executable(FrameTest
  bvh_test.cpp
  camera_test.cpp
  camera_test.h
  device_mock.h
  main.cpp
  plugin_mock.h
  program_mock.h
  uniform_mock.h
  window_factory_test.cpp
  window_factory_test.h
)

target_include_directories(FrameTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_BINARY_DIR}/frame/proto/generated
)

target_link_libraries(FrameTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_LINK_GROUP_END}
)

# In order to remove the tests from the bin folder.
set_target_properties(FrameTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameTest)

add_subdirectory(file)
add_subdirectory(json)
add_subdirectory(opengl)
add_subdirectory(vulkan)

set_property(TARGET FrameTest PROPERTY FOLDER "FrameTest")
