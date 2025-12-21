# Asset Shader Vulkan.

set(_frame_vulkan_shader_sources
  japanese_flag.frag
  japanese_flag.vert
  ray_marching.frag
  ray_marching.vert
  cubemap.frag
  cubemap.vert
  raytracing.vert
  raytracing.frag
  raytracing.comp
)

add_custom_target(AssetShaderVulkan
  SOURCES ${_frame_vulkan_shader_sources}
)

if(Vulkan_GLSLC_EXECUTABLE)
  set(_frame_vulkan_spv_outputs "")
  foreach(_frame_shader IN LISTS _frame_vulkan_shader_sources)
    set(_frame_shader_src "${CMAKE_CURRENT_LIST_DIR}/${_frame_shader}")
    set(_frame_shader_spv "${_frame_shader_src}.spv")
    add_custom_command(
      OUTPUT "${_frame_shader_spv}"
      COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -o "${_frame_shader_spv}" "${_frame_shader_src}"
      DEPENDS "${_frame_shader_src}"
      COMMENT "Compiling Vulkan shader ${_frame_shader}"
      VERBATIM)
    list(APPEND _frame_vulkan_spv_outputs "${_frame_shader_spv}")
  endforeach()
  add_custom_target(AssetShaderVulkanSpv DEPENDS ${_frame_vulkan_spv_outputs})
  add_dependencies(AssetShaderVulkan AssetShaderVulkanSpv)
endif()

set_property(TARGET AssetShaderVulkan PROPERTY FOLDER "FrameAsset/Shader")
