# Frame Vulkan

add_subdirectory(json)

set(FRAME_VULKAN_SOURCES
  buffer.cpp
  buffer.h
  buffer_resources.cpp
  buffer_resources.h
  build_level.cpp
  build_level.h
  command_resources.cpp
  command_resources.h
  mesh_utils.cpp
  mesh_utils.h
  mesh_resources.cpp
  mesh_resources.h
  material.cpp
  material.h
  program.cpp
  program.h
  command_queue.cpp
  command_queue.h
  gpu_memory_manager.cpp
  gpu_memory_manager.h
  scene_state.cpp
  scene_state.h
  scoped_timer.cpp
  scoped_timer.h
  shader_compiler.cpp
  shader_compiler.h
  static_mesh.cpp
  static_mesh.h
  swapchain_resources.cpp
  swapchain_resources.h
  sync_resources.cpp
  sync_resources.h
  texture.cpp
  texture.h
  texture_resources.cpp
  texture_resources.h
  dispatcher.cpp
  debug_callback.cpp
  debug_callback.h
  device.h
  device.cpp
  sdl_vulkan_none.h
  sdl_vulkan_none.cpp
  sdl_vulkan_window.h
  sdl_vulkan_window.cpp
  vulkan_dispatch.h
  window_factory.h
  window_factory.cpp
)

add_library(FrameVulkan
  STATIC
    ${FRAME_VULKAN_SOURCES}
)

target_include_directories(FrameVulkan
  PUBLIC
    ${CMAKE_BINARY_DIR}/frame/proto/generated
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FrameVulkan
  PUBLIC
    FrameVulkanJson
    Frame
    FrameFile
    FrameJson
    Vulkan::Vulkan
    shaderc::shaderc_combined
)

set_property(TARGET FrameVulkan PROPERTY FOLDER "Frame/Vulkan")

if(MSVC)
  target_compile_options(FrameVulkan PRIVATE /FS)
endif()

if(TARGET AssetShaderVulkanSpv)
  add_dependencies(FrameVulkan AssetShaderVulkanSpv)
endif()
