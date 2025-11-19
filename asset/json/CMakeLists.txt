# Asset Json

add_custom_target(AssetJson
  SOURCES
    device_test.json
    editor.json
    new_project_template.json
    equirectangular.json
    japanese_flag.json
    level_test.json
    material_test.json
    program_test.json
    cubemap.json
    ray_marching.json
    raytracing.json
    renderer_test.json
    scene_tree_test.json
)

set_property(TARGET AssetJson PROPERTY FOLDER "FrameAsset/Json")
