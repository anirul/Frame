# Asset Json

add_custom_target(AssetJson
  SOURCES
    depth_normal.json
    device_test.json
    editor.json
    new_project_template.json
    equirectangular.json
    image_based_lighting.json
    japanese_flag.json
    level_test.json
    material_test.json
    point_cloud.json
    program_test.json
    ray_marching.json
    renderer_test.json
    scene_simple.json
    scene_tree_test.json
    shadow.json
    raytracing.json
)

set_property(TARGET AssetJson PROPERTY FOLDER "FrameAsset/Json")
