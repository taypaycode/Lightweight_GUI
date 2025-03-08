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

# Examples
add_executable(simple_form examples/simple_form.c)
target_link_libraries(simple_form lightgui)

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