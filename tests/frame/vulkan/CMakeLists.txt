# Frame Vulkan Test.

add_executable(FrameVulkanTest
  main.cpp
  command_queue_test.cpp
  raytracing_test.cpp
  raytracing_compute_test.cpp
  scene_state_test.cpp
  resource_manager_test.cpp
  window_test.cpp
  window_input_test.cpp
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
    shaderc::shaderc_combined
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    FrameVulkan
    ${FRAME_LINK_GROUP_END}
)

set_target_properties(FrameVulkanTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

include(GoogleTest)
gtest_add_tests(TARGET FrameVulkanTest)

set_property(TARGET FrameVulkanTest PROPERTY FOLDER "FrameTest/Vulkan")

add_executable(FrameVulkanGUITest
  gui_test.cpp
)

target_include_directories(FrameVulkanGUITest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_BINARY_DIR}/external/frame/frame/proto/generated
)

target_link_libraries(FrameVulkanGUITest
  PUBLIC
    GTest::gtest
    GTest::gtest_main
)

set_target_properties(FrameVulkanGUITest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

if(MSVC)
  target_compile_options(FrameVulkanGUITest PRIVATE /utf-8)
endif()

gtest_add_tests(TARGET FrameVulkanGUITest)

set_property(TARGET FrameVulkanGUITest PROPERTY FOLDER "FrameTest/Vulkan")
