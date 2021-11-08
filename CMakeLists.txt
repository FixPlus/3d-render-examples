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
    message(FATAL_ERROR "Linux build configuration is not supported yet")
ENDIF()



IF(WIN32)
    find_library(GLFW_LIBRARY NAMES glfw3dll PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW)
    IF(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "GLFW library not found!")
    ENDIF()
    message(STATUS "Found GLFW: " ${GLFW_LIBRARY})
ELSE()
    # TODO
    message(FATAL_ERROR "Not supported linking of GLFW on this platform!")
ENDIF()




include_directories(include)
include_directories(external)
include_directories(external/glm)

set(RENDER_API_LIB APIDriver)

add_subdirectory(source)

set(PROJECT_LIBS ${RENDER_API_LIB})

add_subdirectory(examples)

IF(NOT glfw_FOUND)
    IF(WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW/glfw3.dll glfw3.dll COPYONLY)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/GLFW/glfw3.dll DESTINATION ${CMAKE_INSTALL_PREFIX})
    ELSE()
        # TODO
        message(FATAl_ERROR "Other platforms don't contain bundled glfw lib yet!")
    ENDIF()
ENDIF()