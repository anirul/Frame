# Frame Vulkan Test.

add_executable(FrameVulkanTest
  main.cpp
  window_test.cpp
)

target_include_directories(FrameVulkanTest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameVulkanTest
  PUBLIC
    GTest::gmock
    GTest::gtest
    FrameVulkan
)

set_target_properties(FrameVulkanTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameVulkanTest)

set_property(TARGET FrameVulkanTest PROPERTY FOLDER "FrameTest/Vulkan")
