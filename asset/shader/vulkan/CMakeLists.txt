# Asset Shader Vulkan.

add_custom_target(AssetShaderVulkan
  SOURCES
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

set_property(TARGET AssetShaderVulkan PROPERTY FOLDER "FrameAsset/Shader")
