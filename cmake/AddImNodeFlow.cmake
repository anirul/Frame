set(_frame_imnodeflow_dir "${CMAKE_CURRENT_LIST_DIR}/../external/ImNodeFlow")
if(NOT EXISTS "${_frame_imnodeflow_dir}/src/ImNodeFlow.cpp")
    set(_frame_imnodeflow_dir "${PROJECT_SOURCE_DIR}/external/ImNodeFlow")
endif()
if(NOT EXISTS "${_frame_imnodeflow_dir}/src/ImNodeFlow.cpp")
    message(FATAL_ERROR "ImNodeFlow source not found. Checked ${CMAKE_CURRENT_LIST_DIR}/../external/ImNodeFlow and ${PROJECT_SOURCE_DIR}/external/ImNodeFlow.")
endif()

add_library(ImNodeFlow STATIC
    "${_frame_imnodeflow_dir}/src/ImNodeFlow.cpp"
)

target_include_directories(ImNodeFlow
    PUBLIC "${_frame_imnodeflow_dir}/include"
)

target_compile_definitions(ImNodeFlow PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(ImNodeFlow PRIVATE imgui::imgui)

unset(_frame_imnodeflow_dir)

