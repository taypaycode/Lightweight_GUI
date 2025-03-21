/**
 * @file program_launcher.c
 * @brief Program Launcher for LightGUI Examples
 * 
 * This example provides a simple GUI for building and running
 * all other example applications in the LightGUI framework.
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
#define mkdir(dir, mode) _mkdir(dir)
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR "/"
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_EXAMPLES 10
#define MAX_PATH 512
#define MAX_CMD_OUTPUT 4096

// Example information structure
typedef struct {
    char name[64];
    char description[256];
    char source_file[MAX_PATH];
    char executable[MAX_PATH];
    LG_WidgetHandle name_label;
    LG_WidgetHandle build_button;
    LG_WidgetHandle run_button;
    LG_WidgetHandle status_label;
} Example;

// Application state
LG_WindowHandle window = NULL;
LG_WidgetHandle output_area = NULL;
LG_WidgetHandle clear_button = NULL;
LG_WidgetHandle rebuild_all_button = NULL;
Example examples[MAX_EXAMPLES];
int example_count = 0;
char current_dir[MAX_PATH];
char build_dir[MAX_PATH];
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
 * @brief Find the project root directory
 * 
 * This function tries to locate the root directory of the project
 * by checking for the existence of key files and directories.
 */
bool find_project_root(char* root_dir, size_t max_length) {
    // Start with the executable directory
    char exec_dir[MAX_PATH];
    
#ifdef _WIN32
    // Get the full path of the executable
    if (GetModuleFileNameA(NULL, exec_dir, MAX_PATH) == 0) {
        return false;
    }
    
    // Find the last backslash to get the directory
    char* last_slash = strrchr(exec_dir, '\\');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at the last backslash
    }
    
    // Go up one level to get out of bin\Debug
    last_slash = strrchr(exec_dir, '\\');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at the last backslash
    }
    
    // Go up another level to get out of bin
    last_slash = strrchr(exec_dir, '\\');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at the last backslash
    }
    
    // At this point, we should be in the build directory
    // Go up one more level to reach the project root
    last_slash = strrchr(exec_dir, '\\');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at the last backslash
    }
#else
    // For Unix systems, use readlink on /proc/self/exe
    if (readlink("/proc/self/exe", exec_dir, MAX_PATH) == -1) {
        return false;
    }
    
    // Find the last slash
    char* last_slash = strrchr(exec_dir, '/');
    if (last_slash) {
        *last_slash = '\0';  // Truncate at the last slash
    }
    
    // Go up three levels
    for (int i = 0; i < 3; i++) {
        last_slash = strrchr(exec_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
    }
#endif

    // Check if this is a valid project root by looking for include directory and CMakeLists.txt
    char test_path[MAX_PATH];
    snprintf(test_path, MAX_PATH, "%s%sinclude%slightgui.h", exec_dir, PATH_SEPARATOR, PATH_SEPARATOR);
    
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(test_path);
    bool is_valid = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    bool is_valid = (stat(test_path, &st) == 0 && S_ISREG(st.st_mode));
#endif

    if (is_valid) {
        strncpy(root_dir, exec_dir, max_length);
        return true;
    }
    
    // If we're here, we couldn't find the project root
    return false;
}

/**
 * @brief Initialize the build environment
 */
bool initialize_build_environment() {
    char project_root[MAX_PATH];
    
    // Find the project root
    if (!find_project_root(project_root, MAX_PATH)) {
        strcpy(cmd_output, "Error: Could not determine project root directory.");
        update_output(cmd_output);
        return false;
    }
    
    // Create or verify build directory
    snprintf(build_dir, MAX_PATH, "%s%sbuild", project_root, PATH_SEPARATOR);
    
    // Check if directory exists
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(build_dir);
    bool exists = (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    bool exists = (stat(build_dir, &st) == 0 && S_ISDIR(st.st_mode));
#endif
    
    if (!exists) {
        // Create directory
#ifdef _WIN32
        if (_mkdir(build_dir) != 0) {
#else
        if (mkdir(build_dir, 0755) != 0) {
#endif
            snprintf(cmd_output, MAX_CMD_OUTPUT, "Error: Could not create build directory: %s", build_dir);
            update_output(cmd_output);
            return false;
        }
        
        // Run CMake
        snprintf(cmd_output, MAX_CMD_OUTPUT, "Creating and configuring build directory...\n");
        update_output(cmd_output);
        
        char cmake_cmd[MAX_PATH * 2];
#ifdef _WIN32
        snprintf(cmake_cmd, sizeof(cmake_cmd), "cd /d \"%s\" && cmake \"%s\"", build_dir, project_root);
#else
        snprintf(cmake_cmd, sizeof(cmake_cmd), "cd \"%s\" && cmake \"%s\"", build_dir, project_root);
#endif
        
        execute_command(cmake_cmd, cmd_output, MAX_CMD_OUTPUT);
        update_output(cmd_output);
    }
    
    return true;
}

/**
 * @brief Build an example
 */
void build_example(int index) {
    if (index < 0 || index >= example_count) return;
    
    // Update status
    LG_SetWidgetText(examples[index].status_label, "Building...");
    
    // Create build command
    char build_cmd[MAX_PATH * 2];
    
#ifdef _WIN32
    snprintf(build_cmd, sizeof(build_cmd), 
             "cd /d \"%s\" && cmake --build . --target %s", 
             build_dir, examples[index].name);
#else
    snprintf(build_cmd, sizeof(build_cmd), 
             "cd \"%s\" && make %s", 
             build_dir, examples[index].name);
#endif
    
    // Execute build command
    bool success = execute_command(build_cmd, cmd_output, MAX_CMD_OUTPUT);
    update_output(cmd_output);
    
    // Update status based on build result
    if (success) {
        LG_SetWidgetText(examples[index].status_label, "Build successful");
    } else {
        LG_SetWidgetText(examples[index].status_label, "Build failed");
    }
}

/**
 * @brief Run an example
 */
void run_example(int index) {
    if (index < 0 || index >= example_count) return;
    
    // Update status
    LG_SetWidgetText(examples[index].status_label, "Running...");
    
    // Create run command
    char executable_path[MAX_PATH * 2];
    
#ifdef _WIN32
    snprintf(executable_path, sizeof(executable_path), 
             "%s%sbin%sDebug%s%s.exe", 
             build_dir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR, examples[index].name);
#else
    snprintf(executable_path, sizeof(executable_path), 
             "%s/bin/%s", 
             build_dir, examples[index].name);
#endif
    
    // Check if executable exists
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(executable_path);
    bool exists = (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    bool exists = (stat(executable_path, &st) == 0 && S_ISREG(st.st_mode));
#endif
    
    if (!exists) {
        snprintf(cmd_output, MAX_CMD_OUTPUT, 
                 "Error: Executable not found: %s\nTry building first.", 
                 executable_path);
        update_output(cmd_output);
        LG_SetWidgetText(examples[index].status_label, "Not built yet");
        return;
    }
    
    // Execute run command
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (!CreateProcess(executable_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        snprintf(cmd_output, MAX_CMD_OUTPUT, "Error: Failed to launch program. Error code: %lu", GetLastError());
        update_output(cmd_output);
        LG_SetWidgetText(examples[index].status_label, "Launch failed");
        return;
    }

    // We don't wait for the process to terminate
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    char run_cmd[MAX_PATH * 2];
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\" &", executable_path);
    bool success = execute_command(run_cmd, cmd_output, MAX_CMD_OUTPUT);
    update_output(cmd_output);
    
    if (!success) {
        LG_SetWidgetText(examples[index].status_label, "Launch failed");
        return;
    }
#endif
    
    LG_SetWidgetText(examples[index].status_label, "Running");
}

/**
 * @brief Rebuild all examples
 */
void rebuild_all() {
    // Update output
    clear_output();
    snprintf(cmd_output, MAX_CMD_OUTPUT, "Rebuilding all examples...\n");
    update_output(cmd_output);
    
    // Create build command
    char build_cmd[MAX_PATH * 2];
    
#ifdef _WIN32
    snprintf(build_cmd, sizeof(build_cmd), 
             "cd /d \"%s\" && cmake --build . --clean-first", build_dir);
#else
    snprintf(build_cmd, sizeof(build_cmd), 
             "cd \"%s\" && make clean && make", build_dir);
#endif
    
    // Execute build command
    bool success = execute_command(build_cmd, cmd_output, MAX_CMD_OUTPUT);
    update_output(cmd_output);
    
    // Update status for all examples
    for (int i = 0; i < example_count; i++) {
        if (success) {
            LG_SetWidgetText(examples[i].status_label, "Build successful");
        } else {
            LG_SetWidgetText(examples[i].status_label, "Build failed");
        }
    }
}

/**
 * @brief Initialize examples list
 */
void initialize_examples() {
    // Simple form example
    strcpy(examples[example_count].name, "simple_form");
    strcpy(examples[example_count].description, "User registration form with validation");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/simple_form.c");
    example_count++;

    // Todo list example
    strcpy(examples[example_count].name, "todo_list");
    strcpy(examples[example_count].description, "Todo list with item management");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/todo_list.c");
    example_count++;

    // Simple paint example
    strcpy(examples[example_count].name, "simple_paint");
    strcpy(examples[example_count].description, "Basic drawing application");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/simple_paint.c");
    example_count++;

    // Calculator example
    strcpy(examples[example_count].name, "calculator");
    strcpy(examples[example_count].description, "Functional calculator with numeric operations");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/calculator.c");
    example_count++;

    // Model viewer example
    strcpy(examples[example_count].name, "model_viewer");
    strcpy(examples[example_count].description, "3D model viewer with OpenGL");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/model_viewer.c");
    example_count++;
    
    // Program launcher (self)
    strcpy(examples[example_count].name, "program_launcher");
    strcpy(examples[example_count].description, "GUI for building and running examples");
    snprintf(examples[example_count].source_file, MAX_PATH, "examples/program_launcher.c");
    example_count++;
}

/**
 * @brief Event callback function
 */
void event_callback(LG_Event* event, void* user_data) {
    if (event->type == LG_EVENT_WINDOW_CLOSE) {
        printf("Window close event received\n");
    } else if (event->type == LG_EVENT_WIDGET_CLICKED) {
        LG_WidgetHandle widget = event->data.widget_clicked.widget;
        
        // Check for clear button
        if (widget == clear_button) {
            clear_output();
            return;
        }
        
        // Check for rebuild all button
        if (widget == rebuild_all_button) {
            rebuild_all();
            return;
        }
        
        // Check for example-specific buttons
        for (int i = 0; i < example_count; i++) {
            if (widget == examples[i].build_button) {
                build_example(i);
                return;
            } else if (widget == examples[i].run_button) {
                run_example(i);
                return;
            }
        }
    }
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Initialize examples
    initialize_examples();
    
    // Create window
    window = LG_CreateWindow("LightGUI Example Launcher", WINDOW_WIDTH, WINDOW_HEIGHT, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create title label
    LG_WidgetHandle title_label = LG_CreateLabel(window, "LightGUI Example Launcher", 
                                                20, 20, WINDOW_WIDTH - 40, 30);
    LG_SetWidgetTextColor(title_label, LG_CreateColor(0, 0, 128, 255));
    
    // Create examples list
    int y_pos = 70;
    for (int i = 0; i < example_count; i++) {
        // Example name label
        char name_with_desc[320];
        snprintf(name_with_desc, sizeof(name_with_desc), 
                 "%s - %s", 
                 examples[i].name, examples[i].description);
        
        examples[i].name_label = LG_CreateLabel(window, name_with_desc, 
                                              20, y_pos, 400, 25);
        
        // Build button
        examples[i].build_button = LG_CreateButton(window, "Build", 
                                                 430, y_pos, 80, 25);
        
        // Run button
        examples[i].run_button = LG_CreateButton(window, "Run", 
                                               520, y_pos, 80, 25);
        
        // Status label
        examples[i].status_label = LG_CreateLabel(window, "Not built yet", 
                                                610, y_pos, 170, 25);
        
        y_pos += 40;
    }
    
    // Create separator
    LG_WidgetHandle separator = LG_CreateLabel(window, "", 
                                              20, y_pos, WINDOW_WIDTH - 40, 1);
    LG_SetWidgetBackgroundColor(separator, LG_CreateColor(200, 200, 200, 255));
    
    y_pos += 20;
    
    // Create control buttons
    clear_button = LG_CreateButton(window, "Clear Output", 
                                  20, y_pos, 120, 30);
    
    rebuild_all_button = LG_CreateButton(window, "Rebuild All", 
                                        150, y_pos, 120, 30);
    
    y_pos += 50;
    
    // Create output area
    output_area = LG_CreateLabel(window, "", 
                                20, y_pos, WINDOW_WIDTH - 40, WINDOW_HEIGHT - y_pos - 20);
    LG_SetWidgetBackgroundColor(output_area, LG_CreateColor(240, 240, 240, 255));
    
    // Initialize build environment
    initialize_build_environment();
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 