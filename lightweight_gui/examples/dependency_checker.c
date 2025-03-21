/**
 * @file dependency_checker.c
 * @brief Dependency Checker for LightGUI Examples
 * 
 * This utility checks for the necessary dependencies and displays
 * detailed diagnostic information for troubleshooting build failures.
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define popen _popen
#define pclose _pclose
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR "/"
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_CMD_OUTPUT 8192

// Widget handles
LG_WindowHandle window;
LG_WidgetHandle title_label;
LG_WidgetHandle opengl_check_button;
LG_WidgetHandle assimp_check_button;
LG_WidgetHandle glew_check_button;
LG_WidgetHandle vcpkg_install_button;
LG_WidgetHandle output_area;
LG_WidgetHandle clear_button;
LG_WidgetHandle cmake_check_button;
LG_WidgetHandle system_check_button;
LG_WidgetHandle generate_cmakelist_button;
LG_WidgetHandle status_label;

char cmd_output[MAX_CMD_OUTPUT];

/**
 * @brief Execute a command and get its output
 */
bool execute_command(const char* command, char* output, size_t output_size) {
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        snprintf(output, output_size, "Error: Failed to execute command: %s", command);
        return false;
    }

    size_t total_read = 0;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL && total_read < output_size - 1) {
        size_t len = strlen(buffer);
        if (total_read + len >= output_size - 1) {
            break;
        }
        strcpy(output + total_read, buffer);
        total_read += len;
    }
    
    output[total_read] = '\0';
    int status = pclose(pipe);
    
    return status == 0;
}

/**
 * @brief Update the output area with text
 */
void update_output(const char* text) {
    LG_SetWidgetText(output_area, text);
}

/**
 * @brief Clear the output area
 */
void clear_output() {
    cmd_output[0] = '\0';
    update_output(cmd_output);
}

/**
 * @brief Check if a file exists
 */
bool file_exists(const char* path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

/**
 * @brief Check if a directory exists
 */
bool directory_exists(const char* path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

/**
 * @brief Check for OpenGL dependencies
 */
void check_opengl() {
    clear_output();
    strcpy(cmd_output, "Checking OpenGL dependencies...\n\n");
    
    // Check for OpenGL header files in common locations
    const char* opengl_headers[] = {
        "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\um\\GL\\gl.h",
        "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.18362.0\\um\\GL\\gl.h",
        "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.17763.0\\um\\GL\\gl.h"
    };
    
    bool header_found = false;
    for (int i = 0; i < sizeof(opengl_headers) / sizeof(opengl_headers[0]); i++) {
        if (file_exists(opengl_headers[i])) {
            strcat(cmd_output, "Found OpenGL header: ");
            strcat(cmd_output, opengl_headers[i]);
            strcat(cmd_output, "\n");
            header_found = true;
            break;
        }
    }
    
    if (!header_found) {
        strcat(cmd_output, "OpenGL headers not found in common locations.\n");
    }
    
    // Check for OpenGL DLLs
    const char* opengl_dlls[] = {
        "C:\\Windows\\System32\\opengl32.dll",
        "C:\\Windows\\System32\\glu32.dll"
    };
    
    for (int i = 0; i < sizeof(opengl_dlls) / sizeof(opengl_dlls[0]); i++) {
        if (file_exists(opengl_dlls[i])) {
            strcat(cmd_output, "Found OpenGL library: ");
            strcat(cmd_output, opengl_dlls[i]);
            strcat(cmd_output, "\n");
        } else {
            strcat(cmd_output, "OpenGL library not found: ");
            strcat(cmd_output, opengl_dlls[i]);
            strcat(cmd_output, "\n");
        }
    }
    
    strcat(cmd_output, "\nRecommendation: OpenGL headers and libraries should be installed with Windows. ");
    strcat(cmd_output, "If they're missing, please update your Windows SDK.\n");
    
    update_output(cmd_output);
}

/**
 * @brief Check for GLEW dependencies
 */
void check_glew() {
    clear_output();
    strcpy(cmd_output, "Checking GLEW dependencies...\n\n");
    
    // Check for GLEW in vcpkg
    strcat(cmd_output, "Checking if GLEW is installed via vcpkg...\n");
    
    char vcpkg_glew_check[MAX_CMD_OUTPUT];
    bool vcpkg_installed = execute_command("vcpkg list glew", vcpkg_glew_check, sizeof(vcpkg_glew_check));
    
    if (vcpkg_installed && strstr(vcpkg_glew_check, "glew")) {
        strcat(cmd_output, "GLEW appears to be installed via vcpkg:\n");
        strcat(cmd_output, vcpkg_glew_check);
    } else {
        strcat(cmd_output, "GLEW not found via vcpkg.\n");
    }
    
    // Check common installation paths
    const char* glew_paths[] = {
        "C:\\vcpkg\\installed\\x64-windows\\include\\GL\\glew.h",
        "C:\\vcpkg\\installed\\x86-windows\\include\\GL\\glew.h",
        "C:\\Program Files\\glew\\include\\GL\\glew.h",
        "C:\\Program Files (x86)\\glew\\include\\GL\\glew.h"
    };
    
    bool found = false;
    for (int i = 0; i < sizeof(glew_paths) / sizeof(glew_paths[0]); i++) {
        if (file_exists(glew_paths[i])) {
            strcat(cmd_output, "Found GLEW header: ");
            strcat(cmd_output, glew_paths[i]);
            strcat(cmd_output, "\n");
            found = true;
        }
    }
    
    if (!found) {
        strcat(cmd_output, "GLEW headers not found in common locations.\n");
    }
    
    strcat(cmd_output, "\nRecommendation: Install GLEW using vcpkg with the command:\n");
    strcat(cmd_output, "vcpkg install glew:x64-windows\n");
    strcat(cmd_output, "or for 32-bit: vcpkg install glew:x86-windows\n");
    
    update_output(cmd_output);
}

/**
 * @brief Check for Assimp dependencies
 */
void check_assimp() {
    clear_output();
    strcpy(cmd_output, "Checking Assimp dependencies...\n\n");
    
    // Check for Assimp in vcpkg
    strcat(cmd_output, "Checking if Assimp is installed via vcpkg...\n");
    
    char vcpkg_assimp_check[MAX_CMD_OUTPUT];
    bool vcpkg_installed = execute_command("vcpkg list assimp", vcpkg_assimp_check, sizeof(vcpkg_assimp_check));
    
    if (vcpkg_installed && strstr(vcpkg_assimp_check, "assimp")) {
        strcat(cmd_output, "Assimp appears to be installed via vcpkg:\n");
        strcat(cmd_output, vcpkg_assimp_check);
    } else {
        strcat(cmd_output, "Assimp not found via vcpkg.\n");
    }
    
    // Check common installation paths
    const char* assimp_paths[] = {
        "C:\\vcpkg\\installed\\x64-windows\\include\\assimp\\Importer.hpp",
        "C:\\vcpkg\\installed\\x86-windows\\include\\assimp\\Importer.hpp",
        "C:\\Program Files\\Assimp\\include\\assimp\\Importer.hpp",
        "C:\\Program Files (x86)\\Assimp\\include\\assimp\\Importer.hpp"
    };
    
    bool found = false;
    for (int i = 0; i < sizeof(assimp_paths) / sizeof(assimp_paths[0]); i++) {
        if (file_exists(assimp_paths[i])) {
            strcat(cmd_output, "Found Assimp header: ");
            strcat(cmd_output, assimp_paths[i]);
            strcat(cmd_output, "\n");
            found = true;
        }
    }
    
    if (!found) {
        strcat(cmd_output, "Assimp headers not found in common locations.\n");
    }
    
    strcat(cmd_output, "\nRecommendation: Install Assimp using vcpkg with the command:\n");
    strcat(cmd_output, "vcpkg install assimp:x64-windows\n");
    strcat(cmd_output, "or for 32-bit: vcpkg install assimp:x86-windows\n");
    
    update_output(cmd_output);
}

/**
 * @brief Check for vcpkg
 */
void check_vcpkg() {
    clear_output();
    strcpy(cmd_output, "Checking vcpkg installation...\n\n");
    
    char vcpkg_version[MAX_CMD_OUTPUT];
    bool vcpkg_installed = execute_command("vcpkg version", vcpkg_version, sizeof(vcpkg_version));
    
    if (vcpkg_installed) {
        strcat(cmd_output, "vcpkg appears to be installed:\n");
        strcat(cmd_output, vcpkg_version);
    } else {
        strcat(cmd_output, "vcpkg not found or not in PATH.\n\n");
        strcat(cmd_output, "To install vcpkg:\n");
        strcat(cmd_output, "1. Clone the repository: git clone https://github.com/microsoft/vcpkg\n");
        strcat(cmd_output, "2. Run bootstrap: .\\vcpkg\\bootstrap-vcpkg.bat\n");
        strcat(cmd_output, "3. Add to PATH: set PATH=%PATH%;C:\\path\\to\\vcpkg\n");
        strcat(cmd_output, "4. Install dependencies: vcpkg install glew:x64-windows assimp:x64-windows\n");
    }
    
    update_output(cmd_output);
}

/**
 * @brief Install dependencies via vcpkg
 */
void install_vcpkg_deps() {
    clear_output();
    strcpy(cmd_output, "Installing dependencies via vcpkg...\n\n");
    update_output(cmd_output);
    
    char install_output[MAX_CMD_OUTPUT];
    bool success = execute_command("vcpkg install glew:x64-windows assimp:x64-windows", 
                                   install_output, sizeof(install_output));
    
    if (success) {
        strcat(cmd_output, "Dependencies installed successfully:\n");
    } else {
        strcat(cmd_output, "Error installing dependencies:\n");
    }
    
    strcat(cmd_output, install_output);
    update_output(cmd_output);
}

/**
 * @brief Check CMake configuration
 */
void check_cmake() {
    clear_output();
    strcpy(cmd_output, "Checking CMake configuration...\n\n");
    
    // Check CMake version
    char cmake_version[MAX_CMD_OUTPUT];
    bool cmake_installed = execute_command("cmake --version", cmake_version, sizeof(cmake_version));
    
    if (cmake_installed) {
        strcat(cmd_output, "CMake version:\n");
        strcat(cmd_output, cmake_version);
        strcat(cmd_output, "\n");
    } else {
        strcat(cmd_output, "CMake not found or not in PATH.\n\n");
        strcat(cmd_output, "Please install CMake from https://cmake.org/download/\n\n");
    }
    
    // Check if CMakeLists.txt contains proper configuration for 3D model viewer
    const char* cmake_file = "..\\CMakeLists.txt";
    FILE* file = fopen(cmake_file, "r");
    if (!file) {
        strcat(cmd_output, "Could not open CMakeLists.txt for analysis.\n");
        update_output(cmd_output);
        return;
    }
    
    char line[1024];
    bool has_opengl = false;
    bool has_glew = false;
    bool has_assimp = false;
    bool has_model_viewer = false;
    
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "find_package(OpenGL)")) has_opengl = true;
        if (strstr(line, "find_package(GLEW)")) has_glew = true;
        if (strstr(line, "find_package(assimp)")) has_assimp = true;
        if (strstr(line, "add_executable(model_viewer")) has_model_viewer = true;
    }
    
    fclose(file);
    
    strcat(cmd_output, "\nCMakeLists.txt analysis:\n");
    strcat(cmd_output, "- OpenGL find_package: ");
    strcat(cmd_output, has_opengl ? "Found\n" : "Not found\n");
    
    strcat(cmd_output, "- GLEW find_package: ");
    strcat(cmd_output, has_glew ? "Found\n" : "Not found\n");
    
    strcat(cmd_output, "- Assimp find_package: ");
    strcat(cmd_output, has_assimp ? "Found\n" : "Not found\n");
    
    strcat(cmd_output, "- model_viewer target: ");
    strcat(cmd_output, has_model_viewer ? "Found\n" : "Not found\n");
    
    update_output(cmd_output);
}

/**
 * @brief Check system information
 */
void check_system() {
    clear_output();
    strcpy(cmd_output, "Collecting system information...\n\n");
    update_output(cmd_output);
    
    // Check Windows version
    char windows_ver[MAX_CMD_OUTPUT];
    execute_command("ver", windows_ver, sizeof(windows_ver));
    strcat(cmd_output, "Windows Version:\n");
    strcat(cmd_output, windows_ver);
    strcat(cmd_output, "\n");
    
    // Check Visual Studio installation
    char vs_info[MAX_CMD_OUTPUT];
    strcat(cmd_output, "Visual Studio Installation:\n");
    if (execute_command("where devenv", vs_info, sizeof(vs_info))) {
        strcat(cmd_output, "Visual Studio found at:\n");
        strcat(cmd_output, vs_info);
    } else {
        strcat(cmd_output, "Visual Studio not found in PATH.\n");
    }
    strcat(cmd_output, "\n");
    
    // Check available compilers
    char compiler_info[MAX_CMD_OUTPUT];
    strcat(cmd_output, "C/C++ Compiler Information:\n");
    if (execute_command("where cl", compiler_info, sizeof(compiler_info))) {
        strcat(cmd_output, "MSVC Compiler found at:\n");
        strcat(cmd_output, compiler_info);
    } else {
        strcat(cmd_output, "MSVC Compiler not found in PATH.\n");
    }
    
    // Check for graphics drivers
    strcat(cmd_output, "\nGraphics Driver Information:\n");
    char gpu_info[MAX_CMD_OUTPUT];
    if (execute_command("wmic path win32_VideoController get Name, DriverVersion", 
                       gpu_info, sizeof(gpu_info))) {
        strcat(cmd_output, gpu_info);
    } else {
        strcat(cmd_output, "Could not retrieve graphics driver information.\n");
    }
    
    update_output(cmd_output);
}

/**
 * @brief Generate a minimal CMakeLists for the model viewer
 */
void generate_minimal_cmakelist() {
    clear_output();
    
    const char* file_path = "model_viewer_minimal.cmake";
    FILE* file = fopen(file_path, "w");
    if (!file) {
        strcpy(cmd_output, "Error: Could not create minimal CMakeLists file.\n");
        update_output(cmd_output);
        return;
    }
    
    // Write a minimal CMake configuration for the model viewer
    fprintf(file, "# Minimal CMakeLists.txt for model_viewer\n");
    fprintf(file, "cmake_minimum_required(VERSION 3.10)\n");
    fprintf(file, "project(ModelViewer VERSION 0.1.0 LANGUAGES C)\n\n");
    
    fprintf(file, "# Set C standard\n");
    fprintf(file, "set(CMAKE_C_STANDARD 99)\n");
    fprintf(file, "set(CMAKE_C_STANDARD_REQUIRED ON)\n\n");
    
    fprintf(file, "# Find required packages\n");
    fprintf(file, "find_package(OpenGL REQUIRED)\n");
    fprintf(file, "find_package(GLEW REQUIRED)\n");
    fprintf(file, "find_package(assimp REQUIRED)\n\n");
    
    fprintf(file, "# Include directories\n");
    fprintf(file, "include_directories(${OPENGL_INCLUDE_DIR})\n");
    fprintf(file, "include_directories(${GLEW_INCLUDE_DIRS})\n");
    fprintf(file, "include_directories(${assimp_INCLUDE_DIRS})\n\n");
    
    fprintf(file, "# Create executable\n");
    fprintf(file, "add_executable(model_viewer examples/model_viewer.c)\n\n");
    
    fprintf(file, "# Link libraries\n");
    fprintf(file, "target_link_libraries(model_viewer\n");
    fprintf(file, "    ${OPENGL_LIBRARIES}\n");
    fprintf(file, "    ${GLEW_LIBRARIES}\n");
    fprintf(file, "    ${assimp_LIBRARIES}\n");
    fprintf(file, ")\n");
    
    fclose(file);
    
    strcpy(cmd_output, "Generated minimal CMakeLists for model_viewer at:\n");
    strcat(cmd_output, file_path);
    strcat(cmd_output, "\n\nYou can try building with this minimal configuration to isolate build issues.\n");
    strcat(cmd_output, "Instructions:\n");
    strcat(cmd_output, "1. Create a new directory: mkdir model_viewer_test\n");
    strcat(cmd_output, "2. Copy the generated file: copy ");
    strcat(cmd_output, file_path);
    strcat(cmd_output, " model_viewer_test\\CMakeLists.txt\n");
    strcat(cmd_output, "3. Copy example file: copy examples\\model_viewer.c model_viewer_test\\\n");
    strcat(cmd_output, "4. Navigate to directory: cd model_viewer_test\n");
    strcat(cmd_output, "5. Configure: cmake .\n");
    strcat(cmd_output, "6. Build: cmake --build .\n");
    
    update_output(cmd_output);
}

/**
 * @brief Event callback function
 */
void event_callback(LG_Event* event, void* user_data) {
    if (event->type == LG_EVENT_WINDOW_CLOSE) {
        printf("Window close event received\n");
    } else if (event->type == LG_EVENT_WIDGET_CLICKED) {
        LG_WidgetHandle widget = event->data.widget_clicked.widget;
        
        if (widget == opengl_check_button) {
            check_opengl();
            LG_SetWidgetText(status_label, "OpenGL dependencies checked");
        } else if (widget == glew_check_button) {
            check_glew();
            LG_SetWidgetText(status_label, "GLEW dependencies checked");
        } else if (widget == assimp_check_button) {
            check_assimp();
            LG_SetWidgetText(status_label, "Assimp dependencies checked");
        } else if (widget == vcpkg_install_button) {
            install_vcpkg_deps();
            LG_SetWidgetText(status_label, "Installation command executed");
        } else if (widget == clear_button) {
            clear_output();
            LG_SetWidgetText(status_label, "Output cleared");
        } else if (widget == cmake_check_button) {
            check_cmake();
            LG_SetWidgetText(status_label, "CMake configuration checked");
        } else if (widget == system_check_button) {
            check_system();
            LG_SetWidgetText(status_label, "System information collected");
        } else if (widget == generate_cmakelist_button) {
            generate_minimal_cmakelist();
            LG_SetWidgetText(status_label, "Minimal CMakeLists.txt generated");
        }
    }
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    window = LG_CreateWindow("Dependency Checker", WINDOW_WIDTH, WINDOW_HEIGHT, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create title
    title_label = LG_CreateLabel(window, "LightGUI Dependency Checker", 
                               20, 20, WINDOW_WIDTH - 40, 30);
    LG_SetWidgetTextColor(title_label, LG_CreateColor(0, 0, 128, 255));
    
    // Create check buttons
    int button_y = 60;
    int button_height = 30;
    int button_spacing = 10;
    
    opengl_check_button = LG_CreateButton(window, "Check OpenGL", 
                                        20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    glew_check_button = LG_CreateButton(window, "Check GLEW", 
                                      20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    assimp_check_button = LG_CreateButton(window, "Check Assimp", 
                                        20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    cmake_check_button = LG_CreateButton(window, "Check CMake Config", 
                                       20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    system_check_button = LG_CreateButton(window, "Check System Info", 
                                        20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    vcpkg_install_button = LG_CreateButton(window, "Install Dependencies", 
                                         20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    generate_cmakelist_button = LG_CreateButton(window, "Generate Minimal CMake", 
                                              20, button_y, 180, button_height);
    button_y += button_height + button_spacing;
    
    clear_button = LG_CreateButton(window, "Clear Output", 
                                 20, WINDOW_HEIGHT - 40, 180, button_height);
    
    // Create output area
    output_area = LG_CreateLabel(window, "Click a button to perform a check...", 
                               220, 60, WINDOW_WIDTH - 240, WINDOW_HEIGHT - 100);
    LG_SetWidgetBackgroundColor(output_area, LG_CreateColor(240, 240, 240, 255));
    
    // Create status label
    status_label = LG_CreateLabel(window, "Ready", 
                                220, WINDOW_HEIGHT - 40, WINDOW_WIDTH - 240, button_height);
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 