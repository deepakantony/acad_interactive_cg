project(interactiveCG)


if(APPLE)
	set(SYSTEM_INCLUDES /System/Library/Frameworks /Library/Frameworks)
	find_library(GLUT_LIBRARY glut)
	find_library(GL_LIBRARY OpenGL)
	find_library(GLUI_LIBRARY GLUI)
	find_library(GLEW_LIBRARY GLEW)
	set(EXTRA_LIBS ${GLUT_LIBRARY} ${GL_LIBRARY} ${GLUI_LIBRARY} ${GLEW_LIBRARY})
#	add_definitions(-m32)
#	set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -m32)
#	set(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}} -m32)
#	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -m32)
endif(APPLE)


subdirs(cube_mapping shadows )
