# Last Monster Standing

add_executable(05-LastMonsterStanding
  WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/asset/json/last_monster_standing.json
)

target_include_directories(05-LastMonsterStanding
  PUBLIC
    examples/04-japanese_flag
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(05-LastMonsterStanding
  PUBLIC
    Frame
    FrameCommon
    FrameGui
    FrameOpenGL
    FrameOpenGLGui
    FrameProto
)

set_property(TARGET 05-LastMonsterStanding PROPERTY FOLDER "Examples")
