# Frame Model Converter

add_executable(FrameModelConverter
  main.cpp
)

target_include_directories(FrameModelConverter
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(FrameModelConverter
  PRIVATE
    assimp::assimp
)

set_property(TARGET FrameModelConverter PROPERTY FOLDER "FrameTools")

