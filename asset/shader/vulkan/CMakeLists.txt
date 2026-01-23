# Asset Shader Vulkan.

set(_frame_vulkan_shader_sources
  japanese_flag.frag
  japanese_flag.vert
  ray_marching.frag
  ray_marching.vert
  cubemap.frag
  cubemap.vert
  raytracing_bvh.vert
  raytracing_bvh.frag
  raytracing_bvh.comp
  raytracing_simple.vert
  raytracing_simple.frag
  raytracing.comp
  raytracing_dual.comp
)

add_custom_target(AssetShaderVulkan
  SOURCES ${_frame_vulkan_shader_sources}
)

if(Vulkan_GLSLC_EXECUTABLE)
  set(_frame_vulkan_spv_dir "${CMAKE_CURRENT_LIST_DIR}/../../cache/shader/vulkan")
  set(_frame_vulkan_spv_outputs "")
  foreach(_frame_shader IN LISTS _frame_vulkan_shader_sources)
    set(_frame_shader_src "${CMAKE_CURRENT_LIST_DIR}/${_frame_shader}")
    set(_frame_shader_spv "${_frame_vulkan_spv_dir}/${_frame_shader}.spv")
    add_custom_command(
      OUTPUT "${_frame_shader_spv}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_frame_vulkan_spv_dir}"
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
set_property(TARGET AssetShaderVulkanSpv PROPERTY FOLDER "FrameAsset/Shader")
