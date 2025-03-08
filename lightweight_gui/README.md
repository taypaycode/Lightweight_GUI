# LightGUI - A Lightweight Cross-Platform GUI Framework

LightGUI is a minimalist C-based GUI toolkit designed to work across multiple operating systems with minimal dependencies. It provides a simple API for creating windows, buttons, text fields, and other common UI elements.

## Features

- Cross-platform support (Windows, Linux)
- Minimal external dependencies
- Simple, intuitive API
- Lightweight and fast rendering
- Event-driven architecture
- Support for common UI widgets

## Building from Source

### Prerequisites

- C compiler (GCC, Clang, or MSVC)
- CMake (version 3.10 or higher)
- Platform-specific dependencies:
  - **Windows**: Windows SDK
  - **Linux**: X11 development libraries (`libx11-dev` package on Debian/Ubuntu)
  - **macOS**: Not yet implemented

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/lightgui.git
cd lightgui

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
```

## Running the Examples

After building, you can run the example applications:

```bash
# From the build directory
./bin/simple_form
```

## Project Structure

- `include/` - Public header files
- `src/` - Core framework source code
- `platform/` - Platform-specific implementations
- `examples/` - Example applications
- `docs/` - Documentation

## API Overview

### Initialization and Cleanup

```c
// Initialize the framework
bool LG_Initialize(void);

// Clean up resources
void LG_Terminate(void);
```

### Window Management

```c
// Create a window
LG_WindowHandle LG_CreateWindow(const char* title, int width, int height, bool resizable);

// Show/hide a window
void LG_ShowWindow(LG_WindowHandle window);
void LG_HideWindow(LG_WindowHandle window);

// Set window title
void LG_SetWindowTitle(LG_WindowHandle window, const char* title);

// Destroy a window
void LG_DestroyWindow(LG_WindowHandle window);
```

### Widget Creation

```c
// Create a button
LG_WidgetHandle LG_CreateButton(LG_WindowHandle window, const char* text, 
                               int x, int y, int width, int height);

// Create a label
LG_WidgetHandle LG_CreateLabel(LG_WindowHandle window, const char* text, 
                              int x, int y, int width, int height);

// Create a text field
LG_WidgetHandle LG_CreateTextField(LG_WindowHandle window, const char* text, 
                                  int x, int y, int width, int height);
```

### Widget Manipulation

```c
// Set/get widget text
void LG_SetWidgetText(LG_WidgetHandle widget, const char* text);
const char* LG_GetWidgetText(LG_WidgetHandle widget);

// Set widget position
void LG_SetWidgetPosition(LG_WidgetHandle widget, int x, int y);

// Set widget size
void LG_SetWidgetSize(LG_WidgetHandle widget, int width, int height);

// Set widget visibility
void LG_SetWidgetVisible(LG_WidgetHandle widget, bool visible);

// Set widget enabled state
void LG_SetWidgetEnabled(LG_WidgetHandle widget, bool enabled);

// Set widget colors
void LG_SetWidgetBackgroundColor(LG_WidgetHandle widget, LG_Color color);
void LG_SetWidgetTextColor(LG_WidgetHandle widget, LG_Color color);
```

### Event Handling

```c
// Set event callback
void LG_SetWindowEventCallback(LG_WindowHandle window, LG_EventCallback callback, void* user_data);

// Run event loop
void LG_RunEventLoop(void);

// Quit event loop
void LG_QuitEventLoop(void);
```

## Simple Example

```c
#include <lightgui.h>
#include <stdio.h>

void event_callback(const LG_Event* event, void* user_data) {
    if (event->type == LG_EVENT_WINDOW_CLOSE) {
        LG_QuitEventLoop();
    }
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    LG_WindowHandle window = LG_CreateWindow("Hello, World!", 400, 300, false);
    LG_SetWindowEventCallback(window, event_callback, NULL);
    
    // Create a button
    LG_WidgetHandle button = LG_CreateButton(window, "Click Me", 150, 120, 100, 30);
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_RunEventLoop();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
}
```

## Extending the Framework

LightGUI is designed to be extensible. To add support for a new platform:

1. Create a new implementation file in the `platform/` directory
2. Implement all the platform-specific functions defined in `lightgui_internal.h`
3. Update the CMakeLists.txt file to include your new platform

## License

This project is licensed under the MIT License - see the LICENSE file for details. 