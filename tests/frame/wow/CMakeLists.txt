target_sources(FrameTest
  PRIVATE
    m2_reader_test.cpp
)

target_link_libraries(FrameTest
  PRIVATE
    FrameWow
)
