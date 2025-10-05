# Asset Shader Vulkan.

add_custom_target(AssetShaderVulkan
  SOURCES
    japanese_flag.frag
    japanese_flag.vert
)

set_property(TARGET AssetShaderVulkan PROPERTY FOLDER "FrameAsset/Shader")
