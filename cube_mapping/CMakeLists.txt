add_library(cube_mapping_util Quad.cpp cone.cpp texture.c)
add_executable(cube_mapping sampleGUI.cpp)
include_directories(. ${SYSTEM_INCLUDES})
target_link_libraries(cube_mapping cube_mapping_util ${EXTRA_LIBS})
