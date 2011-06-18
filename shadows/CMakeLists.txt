add_library(shadows_lib Cube.cpp cone.cpp cubemap.cpp quad.cpp texture.c)
include_directories(${SYSTEM_INCLUDES})
add_executable(shadows opengl3.cpp)
target_link_libraries(shadows shadows_lib ${EXTRA_LIBS})
