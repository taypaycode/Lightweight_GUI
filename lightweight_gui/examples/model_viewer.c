/**
 * @file model_viewer.c
 * @brief 3D Model Viewer using the LightGUI framework and OpenGL
 * 
 * This example demonstrates how to integrate OpenGL with LightGUI to create
 * a simple 3D model viewer for FBX files.
 * 
 * Dependencies:
 * - OpenGL (GL/glew.h, GL/gl.h)
 * - assimp (Open Asset Import Library) for loading 3D models
 * - glm (OpenGL Mathematics) for matrix operations
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Include platform-specific OpenGL headers
#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

// Include assimp headers for model loading
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define CANVAS_WIDTH 800
#define CANVAS_HEIGHT 600
#define MAX_FILENAME 256

// Application state
LG_WindowHandle window = NULL;
LG_WidgetHandle canvas = NULL;
LG_WidgetHandle status_label = NULL;
LG_WidgetHandle load_button = NULL;
LG_WidgetHandle reset_button = NULL;
LG_WidgetHandle wireframe_toggle = NULL;
LG_WidgetHandle rotate_x_plus = NULL;
LG_WidgetHandle rotate_x_minus = NULL;
LG_WidgetHandle rotate_y_plus = NULL;
LG_WidgetHandle rotate_y_minus = NULL;
LG_WidgetHandle zoom_in = NULL;
LG_WidgetHandle zoom_out = NULL;

// OpenGL and 3D model state
GLuint shader_program = 0;
GLuint vao = 0;
GLuint vbo_positions = 0;
GLuint vbo_normals = 0;
GLuint vbo_texcoords = 0;
GLuint ebo = 0;

// Model data
const struct aiScene* scene = NULL;
struct aiVector3D* vertices = NULL;
struct aiVector3D* normals = NULL;
struct aiVector3D* texcoords = NULL;
unsigned int* indices = NULL;
unsigned int num_vertices = 0;
unsigned int num_indices = 0;

// Camera and model transformation
float rotation_x = 0.0f;
float rotation_y = 0.0f;
float zoom = 1.0f;
bool wireframe_mode = false;

// Other state
char model_filename[MAX_FILENAME] = "";
bool model_loaded = false;
bool gl_initialized = false;

// Shader source code
const char* vertex_shader_source = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "    TexCoord = aTexCoord;\n"
    "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\n";

const char* fragment_shader_source = 
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec2 TexCoord;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 objectColor;\n"
    "void main()\n"
    "{\n"
    "    // Ambient\n"
    "    float ambientStrength = 0.2;\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "    // Diffuse\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    // Specular\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * lightColor;\n"
    "    // Result\n"
    "    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n";

/**
 * @brief Initialize OpenGL
 */
bool init_opengl() {
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(err));
        return false;
    }
    
    // Create and compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    // Check for vertex shader compilation errors
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "Vertex shader compilation failed: %s\n", info_log);
        return false;
    }
    
    // Create and compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    
    // Check for fragment shader compilation errors
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "Fragment shader compilation failed: %s\n", info_log);
        return false;
    }
    
    // Create shader program and link shaders
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    // Check for shader program linking errors
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        fprintf(stderr, "Shader program linking failed: %s\n", info_log);
        return false;
    }
    
    // Delete shaders as they're linked into our program now
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_positions);
    glGenBuffers(1, &vbo_normals);
    glGenBuffers(1, &vbo_texcoords);
    glGenBuffers(1, &ebo);
    
    // Set up OpenGL viewport
    glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    return true;
}

/**
 * @brief Clean up OpenGL resources
 */
void cleanup_opengl() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo_positions) glDeleteBuffers(1, &vbo_positions);
    if (vbo_normals) glDeleteBuffers(1, &vbo_normals);
    if (vbo_texcoords) glDeleteBuffers(1, &vbo_texcoords);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (shader_program) glDeleteProgram(shader_program);
    
    // Free model data
    if (vertices) free(vertices);
    if (normals) free(normals);
    if (texcoords) free(texcoords);
    if (indices) free(indices);
    
    // Release the scene
    if (scene) aiReleaseImport(scene);
    
    // Reset state
    vertices = NULL;
    normals = NULL;
    texcoords = NULL;
    indices = NULL;
    scene = NULL;
    num_vertices = 0;
    num_indices = 0;
}

/**
 * @brief Load a 3D model
 */
bool load_model(const char* filename) {
    // Clean up previous model if any
    if (vertices) free(vertices);
    if (normals) free(normals);
    if (texcoords) free(texcoords);
    if (indices) free(indices);
    if (scene) aiReleaseImport(scene);
    
    // Reset state
    vertices = NULL;
    normals = NULL;
    texcoords = NULL;
    indices = NULL;
    scene = NULL;
    num_vertices = 0;
    num_indices = 0;
    
    // Load the model
    scene = aiImportFile(filename, 
                        aiProcess_Triangulate | 
                        aiProcess_GenSmoothNormals | 
                        aiProcess_FlipUVs | 
                        aiProcess_JoinIdenticalVertices);
    
    if (!scene) {
        fprintf(stderr, "Failed to load model: %s\n", aiGetErrorString());
        return false;
    }
    
    // Process the first mesh (for simplicity)
    if (scene->mNumMeshes == 0) {
        fprintf(stderr, "Model has no meshes\n");
        aiReleaseImport(scene);
        scene = NULL;
        return false;
    }
    
    struct aiMesh* mesh = scene->mMeshes[0];
    
    // Allocate memory for vertex data
    num_vertices = mesh->mNumVertices;
    vertices = (struct aiVector3D*)malloc(sizeof(struct aiVector3D) * num_vertices);
    normals = (struct aiVector3D*)malloc(sizeof(struct aiVector3D) * num_vertices);
    texcoords = (struct aiVector3D*)malloc(sizeof(struct aiVector3D) * num_vertices);
    
    // Copy vertex data
    memcpy(vertices, mesh->mVertices, sizeof(struct aiVector3D) * num_vertices);
    memcpy(normals, mesh->mNormals, sizeof(struct aiVector3D) * num_vertices);
    
    // Copy texture coordinates if available
    if (mesh->mTextureCoords[0]) {
        for (unsigned int i = 0; i < num_vertices; i++) {
            texcoords[i].x = mesh->mTextureCoords[0][i].x;
            texcoords[i].y = mesh->mTextureCoords[0][i].y;
            texcoords[i].z = 0.0f;
        }
    } else {
        // Initialize with zeros if no texture coordinates
        memset(texcoords, 0, sizeof(struct aiVector3D) * num_vertices);
    }
    
    // Allocate memory for index data
    num_indices = mesh->mNumFaces * 3; // Assuming triangulated mesh
    indices = (unsigned int*)malloc(sizeof(unsigned int) * num_indices);
    
    // Copy index data
    unsigned int index = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        struct aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices[index++] = face.mIndices[j];
        }
    }
    
    // Bind VAO
    glBindVertexArray(vao);
    
    // Set up vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct aiVector3D) * num_vertices, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct aiVector3D), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Set up vertex normals
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct aiVector3D) * num_vertices, normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct aiVector3D), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Set up texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct aiVector3D) * num_vertices, texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct aiVector3D), (void*)0);
    glEnableVertexAttribArray(2);
    
    // Set up element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices, indices, GL_STATIC_DRAW);
    
    // Unbind VAO
    glBindVertexArray(0);
    
    return true;
}

/**
 * @brief Render the 3D model
 */
void render_model() {
    if (!model_loaded || !gl_initialized) {
        return;
    }
    
    // Clear the color and depth buffers
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use our shader program
    glUseProgram(shader_program);
    
    // Set up transformation matrices (simplified for this example)
    // In a real application, you would use a proper matrix library like GLM
    
    // Set light and material properties
    GLint light_pos_loc = glGetUniformLocation(shader_program, "lightPos");
    GLint view_pos_loc = glGetUniformLocation(shader_program, "viewPos");
    GLint light_color_loc = glGetUniformLocation(shader_program, "lightColor");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    
    glUniform3f(light_pos_loc, 1.0f, 1.0f, 2.0f);
    glUniform3f(view_pos_loc, 0.0f, 0.0f, 3.0f);
    glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform3f(object_color_loc, 0.5f, 0.5f, 0.5f);
    
    // Set wireframe mode if enabled
    if (wireframe_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Draw the model
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Swap buffers and present
    // Note: In a real integration, we'd need platform-specific code here
    // to manage the OpenGL context within our LightGUI window
}

/**
 * @brief Open a file dialog to select an FBX file
 * 
 * Note: This is a simplified placeholder. In a real application,
 * you would use platform-specific file dialog APIs.
 */
bool open_file_dialog(char* filename, size_t max_length) {
    // For simplicity, we'll just use a hardcoded filename
    // In a real app, you would show a file dialog
    strncpy(filename, "models/cube.fbx", max_length);
    return true;
}

/**
 * @brief Update the status label
 */
void update_status(const char* message) {
    LG_SetWidgetText(status_label, message);
}

/**
 * @brief Reset the camera view
 */
void reset_view() {
    rotation_x = 0.0f;
    rotation_y = 0.0f;
    zoom = 1.0f;
}

/**
 * @brief Load a model file
 */
void load_model_file() {
    if (open_file_dialog(model_filename, MAX_FILENAME)) {
        char status_message[512];
        
        if (load_model(model_filename)) {
            snprintf(status_message, sizeof(status_message), 
                     "Loaded: %s (%u vertices, %u triangles)", 
                     model_filename, num_vertices, num_indices / 3);
            model_loaded = true;
        } else {
            snprintf(status_message, sizeof(status_message), 
                     "Failed to load: %s", model_filename);
            model_loaded = false;
        }
        
        update_status(status_message);
    }
}

/**
 * @brief Toggle wireframe mode
 */
void toggle_wireframe() {
    wireframe_mode = !wireframe_mode;
    
    if (wireframe_mode) {
        update_status("Wireframe mode: ON");
    } else {
        update_status("Wireframe mode: OFF");
    }
}

/**
 * @brief Event callback function
 */
void event_callback(LG_Event* event, void* user_data) {
    switch (event->type) {
        case LG_EVENT_WINDOW_CLOSE:
            printf("Window close event received\n");
            break;
            
        case LG_EVENT_WIDGET_CLICKED:
            {
                LG_WidgetHandle widget = event->data.widget_clicked.widget;
                
                if (widget == load_button) {
                    load_model_file();
                } else if (widget == reset_button) {
                    reset_view();
                    update_status("View reset");
                } else if (widget == wireframe_toggle) {
                    toggle_wireframe();
                } else if (widget == rotate_x_plus) {
                    rotation_x += 10.0f;
                } else if (widget == rotate_x_minus) {
                    rotation_x -= 10.0f;
                } else if (widget == rotate_y_plus) {
                    rotation_y += 10.0f;
                } else if (widget == rotate_y_minus) {
                    rotation_y -= 10.0f;
                } else if (widget == zoom_in) {
                    zoom *= 1.1f;
                } else if (widget == zoom_out) {
                    zoom *= 0.9f;
                }
            }
            break;
            
        case LG_EVENT_MOUSE_MOVE:
            // Handle mouse drag for model rotation
            if (event->data.mouse_move.button_pressed[LG_MOUSE_BUTTON_LEFT]) {
                rotation_x += event->data.mouse_move.delta_y * 0.5f;
                rotation_y += event->data.mouse_move.delta_x * 0.5f;
            }
            break;
            
        case LG_EVENT_MOUSE_BUTTON:
            // Handle mouse wheel for zooming
            if (event->data.mouse_button.button == LG_MOUSE_BUTTON_WHEEL_UP) {
                zoom *= 1.1f;
            } else if (event->data.mouse_button.button == LG_MOUSE_BUTTON_WHEEL_DOWN) {
                zoom *= 0.9f;
            }
            break;
    }
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    window = LG_CreateWindow("3D Model Viewer", WINDOW_WIDTH, WINDOW_HEIGHT, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create canvas for OpenGL rendering
    canvas = LG_CreateCanvas(window, 10, 10, CANVAS_WIDTH, CANVAS_HEIGHT);
    
    // Create UI controls
    int button_x = CANVAS_WIDTH + 20;
    int button_y = 10;
    int button_width = 180;
    int button_height = 30;
    int button_spacing = 10;
    
    load_button = LG_CreateButton(window, "Load Model", button_x, button_y, button_width, button_height);
    button_y += button_height + button_spacing;
    
    reset_button = LG_CreateButton(window, "Reset View", button_x, button_y, button_width, button_height);
    button_y += button_height + button_spacing;
    
    wireframe_toggle = LG_CreateButton(window, "Toggle Wireframe", button_x, button_y, button_width, button_height);
    button_y += button_height + button_spacing * 2;
    
    // Rotation controls
    LG_CreateLabel(window, "Rotation:", button_x, button_y, button_width, 20);
    button_y += 25;
    
    rotate_x_plus = LG_CreateButton(window, "X+", button_x, button_y, 85, button_height);
    rotate_x_minus = LG_CreateButton(window, "X-", button_x + 95, button_y, 85, button_height);
    button_y += button_height + button_spacing;
    
    rotate_y_plus = LG_CreateButton(window, "Y+", button_x, button_y, 85, button_height);
    rotate_y_minus = LG_CreateButton(window, "Y-", button_x + 95, button_y, 85, button_height);
    button_y += button_height + button_spacing * 2;
    
    // Zoom controls
    LG_CreateLabel(window, "Zoom:", button_x, button_y, button_width, 20);
    button_y += 25;
    
    zoom_in = LG_CreateButton(window, "Zoom In", button_x, button_y, 85, button_height);
    zoom_out = LG_CreateButton(window, "Zoom Out", button_x + 95, button_y, 85, button_height);
    button_y += button_height + button_spacing * 2;
    
    // Status label
    status_label = LG_CreateLabel(window, "Ready to load a model", 10, CANVAS_HEIGHT + 20, WINDOW_WIDTH - 20, 20);
    
    // Initialize OpenGL
    gl_initialized = init_opengl();
    if (!gl_initialized) {
        update_status("Failed to initialize OpenGL");
    }
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    cleanup_opengl();
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 