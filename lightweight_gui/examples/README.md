# LightGUI Example Applications

This directory contains example applications built with the LightGUI framework, demonstrating various features and use cases.

## Available Examples

### 1. Simple Form (`simple_form.c`)

A user registration form that demonstrates:
- Basic widgets (labels, text fields, buttons)
- Event handling
- Form validation
- Visual feedback

![Simple Form Screenshot](../docs/images/simple_form.png)

### 2. Todo List (`todo_list.c`)

A todo list application that demonstrates:
- Dynamic widget creation and destruction
- List management
- Checkboxes
- Status tracking

![Todo List Screenshot](../docs/images/todo_list.png)

### 3. Simple Paint (`simple_paint.c`)

A basic drawing application that demonstrates:
- Mouse tracking
- Custom rendering
- Tool selection
- Color palette

![Simple Paint Screenshot](../docs/images/simple_paint.png)

### 4. Calculator (`calculator.c`)

A functional calculator that demonstrates:
- Grid layouts
- Numerical input handling
- Complex application logic
- Keyboard shortcuts

![Calculator Screenshot](../docs/images/calculator.png)

### 5. 3D Model Viewer (`model_viewer.c`)

A 3D model viewer that demonstrates:
- OpenGL integration with LightGUI
- Loading and rendering 3D models (FBX format)
- Camera controls (rotation, zoom)
- Interactive model manipulation

![Model Viewer Screenshot](../docs/images/model_viewer.png)

#### Dependencies

The 3D Model Viewer example requires additional dependencies:
- OpenGL
- GLEW (OpenGL Extension Wrangler)
- Assimp (Open Asset Import Library)

To install these dependencies:

**Windows (with vcpkg):**
```bash
vcpkg install opengl glew assimp
```

**Linux:**
```bash
sudo apt-get install libgl1-mesa-dev libglew-dev libassimp-dev
```

**macOS:**
```bash
brew install glew assimp
```

### 6. Program Launcher (`program_launcher.c`)

A utility application that makes it easy to build and run all LightGUI example programs:
- Lists all available example applications
- Provides buttons to build and run each example
- Allows rebuilding all examples at once
- Displays build output and status information
- Works cross-platform (Windows, Linux, macOS)

![Program Launcher Screenshot](../docs/images/program_launcher.png)

The program launcher eliminates the need to use the command line for building and testing examples, making development and experimentation more convenient.

## Building and Running Examples

You can build all examples by running:

```bash
# From the root directory
mkdir build
cd build
cmake ..
cmake --build .
```

The compiled executables will be placed in the `bin` directory. You can run each example by executing:

```bash
# Windows
bin\Debug\simple_form.exe
bin\Debug\todo_list.exe
bin\Debug\simple_paint.exe
bin\Debug\calculator.exe
bin\Debug\model_viewer.exe
bin\Debug\program_launcher.exe

# macOS/Linux
./bin/simple_form
./bin/todo_list
./bin/simple_paint
./bin/calculator
./bin/model_viewer
./bin/program_launcher
```

### Quick Start

For the easiest way to build and run examples, simply build and run the program launcher:

```bash
# From the root directory
mkdir build
cd build
cmake ..
cmake --build . --target program_launcher
bin\Debug\program_launcher.exe  # Windows
./bin/program_launcher          # macOS/Linux
```

This will provide a graphical interface for building and running all other examples.

## Creating Your Own Applications

These examples provide a good starting point for creating your own applications. Here's a basic template:

```c
#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>

// Event callback function
void event_callback(LG_Event* event, void* user_data) {
    // Handle events here
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    LG_WindowHandle window = LG_CreateWindow("My Application", 800, 600, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create widgets
    // ...
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
}
```

For more detailed information, check the API documentation in the `/docs` directory. 