# Frame Proto.

add_library(FrameProto
  STATIC
    ParseMaterial.cpp
    ParseMaterial.h
    ParsePixel.cpp
    ParsePixel.h
    ParseProgram.cpp
    ParseProgram.h
    ParseSceneTree.cpp
    ParseSceneTree.h
    ParseTexture.cpp
    ParseTexture.h
    ParseUniform.cpp
    ParseUniform.h
    ParseLevel.cpp
    ParseLevel.h
    Proto.cpp
    Proto.h
    Level.proto
    Material.proto
    Math.proto
    Pixel.proto
    Program.proto
    Scene.proto
    Size.proto
    Texture.proto
    Uniform.proto
)

target_include_directories(FrameProto 
  PUBLIC 
    ${CMAKE_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# I still doesn't know how to pass the path correctly to the generate in case
# there is any external include.
protobuf_generate(TARGET FrameProto LANGUAGE cpp)

target_link_libraries(FrameProto 
  PUBLIC
    Frame
    FrameOpenGL
    glm::glm
    protobuf::libprotoc
    protobuf::libprotobuf
)

# Get all propreties that cmake supports
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
    
    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif()
    
function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()
    
function(print_target_properties target)
    if(NOT TARGET ${target})
      message(STATUS "There is no target named '${target}'")
      return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
    endforeach()
endfunction()

print_target_properties(glm::glm)

set_property(TARGET FrameProto PROPERTY FOLDER "Frame")

add_subdirectory(Test)
