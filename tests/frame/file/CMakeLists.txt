# Frame File Test.

add_executable(FrameFileTest
  file_system_test.cpp
  file_system_test.h
  image_test.cpp
  image_test.h
  main.cpp
  obj_test.cpp
  obj_test.h
  ply_test.cpp
  ply_test.h
)

target_include_directories(FrameFileTest
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
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

# In order to remove the tests from the bin folder.
set_target_properties(FrameFileTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameFileTest)

set_property(TARGET FrameFileTest PROPERTY FOLDER "FrameTest")
