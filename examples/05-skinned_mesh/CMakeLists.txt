# Skinned Mesh

add_executable(05_SkinnedMesh
  WIN32
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/json/skinned_mesh.json
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/model/fox/Fox.glb
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/model/fox/Texture.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/model/fox/README.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/model/fox/LICENSE.md
    ${CMAKE_CURRENT_SOURCE_DIR}/../../asset/model/fox/metadata.json
)

target_include_directories(05_SkinnedMesh
  PUBLIC
    examples/05-skinned_mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(05_SkinnedMesh
  PUBLIC
    ${FRAME_LINK_GROUP_BEGIN}
    ${FRAME_CORE_LIBS}
    ${FRAME_VULKAN_LIBS}
    FrameCommon
    ${FRAME_LINK_GROUP_END}
)

set_property(TARGET 05_SkinnedMesh PROPERTY FOLDER "FrameExamples")
