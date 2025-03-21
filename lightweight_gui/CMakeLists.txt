cmake_minimum_required(VERSION 3.10)
project(LightGUI VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Detect platform
if(WIN32)
    set(PLATFORM_SOURCES platform/windows.c)
    set(PLATFORM_LIBS user32 gdi32 comctl32)
    add_definitions(-D_WIN32)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES platform/linux.c)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    set(PLATFORM_LIBS ${X11_LIBRARIES})
    add_definitions(-D__linux__)
elseif(APPLE)
    # macOS implementation would go here
    message(FATAL_ERROR "macOS platform not implemented yet")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Library sources
set(LIGHTGUI_SOURCES
    src/lightgui.c
    ${PLATFORM_SOURCES}
)

# Create library
add_library(lightgui STATIC ${LIGHTGUI_SOURCES})
target_link_libraries(lightgui ${PLATFORM_LIBS})

# Basic Examples - No special dependencies
add_executable(simple_form examples/simple_form.c)
target_link_libraries(simple_form lightgui)

add_executable(todo_list examples/todo_list.c)
target_link_libraries(todo_list lightgui)

add_executable(calculator examples/calculator.c)
target_link_libraries(calculator lightgui)

add_executable(program_launcher examples/program_launcher.c)
target_link_libraries(program_launcher lightgui)

# Add dependency checker tool - useful for debugging dependency issues
add_executable(dependency_checker examples/dependency_checker.c)
target_link_libraries(dependency_checker lightgui)

# Simple Paint Example - Using canvas for custom drawing
add_executable(simple_paint examples/simple_paint.c)
target_link_libraries(simple_paint lightgui)
if(WIN32)
    # Add GDI+ library for advanced drawing on Windows
    target_link_libraries(simple_paint gdiplus)
endif()

# 3D Model Viewer Example with OpenGL and Assimp
# This example requires additional dependencies, so we'll
# make it optional and only build if the dependencies are found

# Find OpenGL
find_package(OpenGL)
find_package(GLEW)
find_package(assimp)

if(OPENGL_FOUND AND GLEW_FOUND AND assimp_FOUND)
    add_executable(model_viewer examples/model_viewer.c)
    target_include_directories(model_viewer PRIVATE 
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${assimp_INCLUDE_DIRS}
    )
    target_link_libraries(model_viewer 
        lightgui 
        ${OPENGL_LIBRARIES} 
        ${GLEW_LIBRARIES}
        ${assimp_LIBRARIES}
    )
    
    # On Windows, we need to ensure the dependencies are found
    if(WIN32)
        # Make sure we're linking with the correct OpenGL libraries
        target_link_libraries(model_viewer opengl32 glu32)
        
        # Add preprocessor definition to use the GLEW static library
        target_compile_definitions(model_viewer PRIVATE GLEW_STATIC)
    endif()
    
    message(STATUS "3D Model Viewer will be built (OpenGL, GLEW, and Assimp found)")
else()
    message(STATUS "3D Model Viewer will not be built (missing dependencies)")
    message(STATUS "  OpenGL found: ${OPENGL_FOUND}")
    message(STATUS "  GLEW found: ${GLEW_FOUND}")
    message(STATUS "  Assimp found: ${assimp_FOUND}")
    message(STATUS "Install these dependencies to build the 3D Model Viewer")
    message(STATUS "  Windows: Use vcpkg to install required packages:")
    message(STATUS "    vcpkg install glew:x64-windows assimp:x64-windows")
    message(STATUS "  Linux: sudo apt-get install libglew-dev libassimp-dev")
endif()

# Installation
install(TARGETS lightgui
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES include/lightgui.h DESTINATION include)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # Replace variables inside @@ with the current values
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # Add a custom target to run Doxygen
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif() 