file(GLOB RENDER_API_SOURCES *.cpp *.h  glfw_window_impl/*.h glfw_window_impl/*.cpp)

set(COMMON_RENDER_HEADERS ../include/RenderInterface.h WindowInterface.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB VULKAN_IMPL_SOURCES vulkan_impl/*.h vulkan_impl/*.cpp vulkan_impl/util/*.cpp vulkan_impl/util/*.h)
file(GLOB OPENGL_IMPL_SOURCES opengl_impl/*.h opengl_impl/*.cpp)
file(GLOB IMGUI_IMPL_SOURCES imgui_impl/*.h imgui_impl/*.cpp)

file(GLOB IMGUI_SOURCE ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp ${PROJECT_SOURCE_DIR}/external/imgui/*.h)

option(LINK_RENDER_API_SHARED "link shared library for APIDriver instead of static one" OFF)

if(LINK_RENDER_API_SHARED AND UNIX)
  add_compile_options(-fPIC)
endif()

add_library(IMGUI_SOURCE_LIB OBJECT ${IMGUI_SOURCE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

add_library(VULKAN_IMPL_LIB OBJECT ${VULKAN_IMPL_SOURCES} ${COMMON_HEADERS})
add_library(OPENGL_IMPL_LIB OBJECT ${OPENGL_IMPL_SOURCES} ${COMMON_HEADERS})
add_library(IMGUI_IMPL_LIB OBJECT ${IMGUI_IMPL_SOURCES}   ${COMMON_HEADERS})

if(LINK_RENDER_API_SHARED)
    add_library(${RENDER_API_LIB} SHARED ${RENDER_API_SOURCES})
else()
    add_library(${RENDER_API_LIB} ${RENDER_API_SOURCES})
endif()

include(../cmake/compileShaders.cmake)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/internal)
compileShaders(${CMAKE_CURRENT_SOURCE_DIR}/internal_shaders ${PROJECT_BINARY_DIR}/internal ${RENDER_API_LIB} ${CMAKE_INSTALL_PREFIX}/internal)
file(GLOB INTERNAL_SHADERS internal_shaders/*.frag internal_shaders/*.vert)

foreach(OGL_SHADER IN LISTS INTERNAL_SHADERS)
    configure_file(${OGL_SHADER} ${PROJECT_BINARY_DIR}/internal COPYONLY)
endforeach()

target_link_libraries(${RENDER_API_LIB} VULKAN_IMPL_LIB OPENGL_IMPL_LIB IMGUI_IMPL_LIB IMGUI_SOURCE_LIB ${Vulkan_LIBRARY} ${GLFW_LIBRARY} ${OpenGL_LIBRARY} ${GLEW_LIBRARY})
install(TARGETS ${RENDER_API_LIB}
        DESTINATION ${CMAKE_INSTALL_PREFIX})
