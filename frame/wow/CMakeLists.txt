add_library(FrameWow
  STATIC
    m2_reader.cpp
    m2_reader.h
)

target_include_directories(FrameWow
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

set_property(TARGET FrameWow PROPERTY FOLDER "Frame/WoW")
