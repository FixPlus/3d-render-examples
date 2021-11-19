cmake_minimum_required(VERSION 3.17)
project(RenderAPITest)

set(CMAKE_CXX_STANDARD 20)


include(cmake/find_vulkan.cmake)

find_package(OpenGL REQUIRED)

IF(WIN32)
    set(OpenGL_LIBRARY OpenGL32.lib)
    find_library(GLEW_LIBRARY NAMES glew32 PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLEW/lib/Release/x64)
    if(NOT GLEW_LIBRARY)
        message(FATAL_ERROR "GLEW library not found!")
    endif()
    message(STATUS "Found GLEW: " ${GLEW_LIBRARY})
    include_directories(external/GLEW/include)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLEW/bin/Release/x64/glew32.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLEW/bin/Release/x64/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
ELSE()
    find_package(GLEW REQUIRED)
    set(OpenGL_LIBRARY ${OPENGL_LIBRARIES})
    set(GLEW_LIBRARY ${GLEW_LIBRARIES})
    include_directories($(GLEW_INCLUDE_DIRS))
ENDIF()



IF(WIN32)
    find_library(GLFW_LIBRARY NAMES glfw3dll PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW)
    IF(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "GLFW library not found!")
    ENDIF()
    message(STATUS "Found GLFW: " ${GLFW_LIBRARY})
ELSE()
    find_package(glfw3 3.3 REQUIRED)
    set(GLFW_LIBRARY glfw)
ENDIF()




include_directories(include)
include_directories(external)
include_directories(external/glm)

set(RENDER_API_LIB APIDriver)

add_subdirectory(source)

set(PROJECT_LIBS ${RENDER_API_LIB})

add_subdirectory(examples)

IF(WIN32)
  IF(NOT glfw_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW/glfw3.dll glfw3.dll COPYONLY)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW/glfw3.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
  ENDIF()
ENDIF()
