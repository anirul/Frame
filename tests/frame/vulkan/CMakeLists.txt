# Frame Vulkan Test.

add_executable(FrameVulkanTest
  main.cpp
  window_test.cpp
)

target_include_directories(FrameVulkanTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameVulkanTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    FrameVulkanData
    ${FRAME_LINK_GROUP_END}
)

set_target_properties(FrameVulkanTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameVulkanTest)

set_property(TARGET FrameVulkanTest PROPERTY FOLDER "FrameTest/Vulkan")
