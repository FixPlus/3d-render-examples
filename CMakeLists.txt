cmake_minimum_required(VERSION 3.17)
project(RenderAPITest)

set(CMAKE_CXX_STANDARD 20)


include(cmake/find_vulkan.cmake)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
set(OpenGL_LIBRARY ${OPENGL_LIBRARIES})
set(GLEW_LIBRARY GLEW::glew)

include_directories($(GLEW_INCLUDE_DIRS))

find_package(glfw3 3.3 REQUIRED)
set(GLFW_LIBRARY glfw)

include_directories(include)
include_directories(external)
include_directories(external/glm)

set(RENDER_API_LIB APIDriver)

add_subdirectory(source)

set(PROJECT_LIBS ${RENDER_API_LIB})

add_subdirectory(examples)
