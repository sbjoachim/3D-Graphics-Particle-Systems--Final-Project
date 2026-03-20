cmake_minimum_required(VERSION 3.16)

project(ParticleSystems)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Source Files
# ---------------------------------------------------------
set(SOURCES
    main.cpp
    src/camera.cpp
    src/particle.cpp
)

set(HEADERS
    include/camera.h
    include/particle.h
)

# Shader files (not compiled, but added for IDE visibility)
set(SHADERS
    resource/basic.vert
    resource/basic.frag
    resource/particle.vert
    resource/particle.frag
    resource/ground.vert
    resource/ground.frag
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${SHADERS}
)

# ---------------------------------------------------------
# Include Directories (target-based)
# ---------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/resource
)

# ---------------------------------------------------------
# OpenGL
# ---------------------------------------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# ---------------------------------------------------------
# External Libraries (GLEW, GLFW, SOIL)
# ---------------------------------------------------------

# Path to your manually downloaded libraries
set(LIBRARY_PATH "C:/Users/Brian Joachim/Desktop/T4/libraries")

if(WIN32)

    # Static GLEW requires this definition
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)

    find_library(GLEW_LIBRARY glew32s HINTS ${LIBRARY_PATH}/lib)
    find_library(GLFW_LIBRARY glfw3 HINTS ${LIBRARY_PATH}/lib)
    find_library(SOIL_LIBRARY SOIL HINTS ${LIBRARY_PATH}/lib)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${LIBRARY_PATH}/include
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GLEW_LIBRARY}
        ${GLFW_LIBRARY}
        ${SOIL_LIBRARY}
    )

else()  # Linux / Mac

    find_package(GLEW REQUIRED)
    find_package(GLFW3 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        GLEW::GLEW
        glfw
        SOIL
    )

endif()

# ---------------------------------------------------------
# Debug postfix for Visual Studio
# ---------------------------------------------------------
if(WIN32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES DEBUG_POSTFIX "_d"
    )
endif()
