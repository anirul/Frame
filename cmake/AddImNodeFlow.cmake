add_library(ImNodeFlow STATIC
    ${CMAKE_SOURCE_DIR}/external/ImNodeFlow/src/ImNodeFlow.cpp
)

target_include_directories(ImNodeFlow
    PUBLIC ${CMAKE_SOURCE_DIR}/external/ImNodeFlow/include
)

target_compile_definitions(ImNodeFlow PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(ImNodeFlow PRIVATE imgui::imgui)

