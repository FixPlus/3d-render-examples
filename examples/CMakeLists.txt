function(buildExample EXAMPLE_NAME)
    set(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
    file(GLOB SOURCE  ${EXAMPLE_FOLDER}/*.cpp ${EXAMPLE_FOLDER}/*.h)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
    add_executable(${EXAMPLE_NAME} ${SOURCE})
    target_link_libraries(${EXAMPLE_NAME} ${PROJECT_LIBS})
    install(TARGETS ${EXAMPLE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
endfunction()



set(EXAMPLES
        basic
        just_attributes
        texture
        uniform_buffer
        model)


file(GLOB COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/common/*.h)

add_library(EXAMPLE_COMMON_LIB OBJECT common.h common.cpp ${COMMON_SOURCES})
list(APPEND PROJECT_LIBS EXAMPLE_COMMON_LIB)
include(../cmake/compileShaders.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(../external/tiny_gltf)

foreach(EXAMPLE ${EXAMPLES})
    buildExample(${EXAMPLE})

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}/shaders)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/shaders/${EXAMPLE})
        compileShaders(${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}/shaders/glsl-v ${PROJECT_BINARY_DIR}/shaders/${EXAMPLE} ${EXAMPLE} ${CMAKE_INSTALL_PREFIX}/shaders/${EXAMPLE})

        file(GLOB OPENGL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}/shaders/glsl/*.*)

        foreach(OGL_SHADER IN LISTS OPENGL_SHADERS)
            configure_file(${OGL_SHADER} ${PROJECT_BINARY_DIR}/shaders/${EXAMPLE} COPYONLY)
        endforeach()

        install(FILES ${OPENGL_SHADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/shaders/${EXAMPLE})
    endif()
endforeach()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/shaders)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/shaders/common)
    compileShaders(${CMAKE_CURRENT_SOURCE_DIR}/common/shaders/glsl-v ${PROJECT_BINARY_DIR}/shaders/common EXAMPLE_COMMON_LIB ${CMAKE_INSTALL_PREFIX}/shaders/common)

    file(GLOB COMMON_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/common/shaders/glsl/*.*)

    foreach(OGL_SHADER IN LISTS COMMON_SHADERS)
        configure_file(${OGL_SHADER} ${PROJECT_BINARY_DIR}/shaders/common COPYONLY)
    endforeach()

    install(FILES ${COMMON_SHADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/shaders/common)
endif()