/**
 * @file lightgui.h
 * @brief Main header file for the LightGUI framework
 * 
 * This header provides the public API for the LightGUI framework,
 * a lightweight cross-platform GUI toolkit written in C.
 */

#ifndef LIGHTGUI_H
#define LIGHTGUI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                              Version Information                          */
/* ========================================================================= */

#define LIGHTGUI_VERSION_MAJOR 0
#define LIGHTGUI_VERSION_MINOR 1
#define LIGHTGUI_VERSION_PATCH 0

/* ========================================================================= */
/*                              Type Definitions                             */
/* ========================================================================= */

/**
 * @brief Rectangle structure
 */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} LG_Rect;

/**
 * @brief Color structure (RGBA)
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} LG_Color;

/**
 * @brief Event types
 */
typedef enum {
    LG_EVENT_MOUSE_MOVE,
    LG_EVENT_MOUSE_BUTTON,
    LG_EVENT_KEY,
    LG_EVENT_WINDOW_RESIZE,
    LG_EVENT_WINDOW_CLOSE,
    LG_EVENT_WIDGET_CLICKED
} LG_EventType;

/**
 * @brief Mouse button identifiers
 */
typedef enum {
    LG_MOUSE_BUTTON_LEFT,
    LG_MOUSE_BUTTON_RIGHT,
    LG_MOUSE_BUTTON_MIDDLE
} LG_MouseButton;

/**
 * @brief Widget types
 */
typedef enum {
    LG_WIDGET_BUTTON,
    LG_WIDGET_LABEL,
    LG_WIDGET_TEXTFIELD,
    LG_WIDGET_CHECKBOX,
    LG_WIDGET_SLIDER,
    LG_WIDGET_PANEL
} LG_WidgetType;

/* Forward declarations for internal structures */
struct LG_Widget;
struct LG_Window;

/* Opaque handle types */
typedef struct LG_Window* LG_WindowHandle;
typedef struct LG_Widget* LG_WidgetHandle;

/**
 * @brief Widget list structure
 */
typedef struct {
    LG_WidgetHandle* widgets;
    size_t count;
    size_t capacity;
} LG_WidgetList;

/**
 * @brief Window structure
 */
struct LG_Window {
    char* title;
    int width;
    int height;
    bool visible;
    bool resizable;
    LG_WidgetList widgets;
    void (*event_callback)(const struct LG_Event* event, void* user_data);
    void* user_data;
    void* platform_data;  // Platform-specific data
};

/**
 * @brief Widget structure
 */
struct LG_Widget {
    LG_WidgetType type;
    LG_WindowHandle window;
    LG_Rect rect;
    char* text;
    bool visible;
    bool enabled;
    LG_Color bg_color;
    LG_Color text_color;
    int id;  // Add an ID field for widget identification
    void* platform_data;
    void* user_data;
};

/**
 * @brief Window list structure
 */
typedef struct {
    LG_WindowHandle* windows;
    size_t count;
    size_t capacity;
} LG_WindowList;

/**
 * @brief Mouse button event
 */
typedef struct {
    LG_MouseButton button;
    bool pressed;
    int x;
    int y;
} LG_MouseButtonEvent;

/**
 * @brief Mouse move event
 */
typedef struct {
    int x;
    int y;
    int delta_x;
    int delta_y;
} LG_MouseMoveEvent;

/**
 * @brief Key event
 */
typedef struct {
    int key_code;
    bool pressed;
    bool ctrl;
    bool shift;
    bool alt;
} LG_KeyEvent;

/**
 * @brief Window resize event
 */
typedef struct {
    int width;
    int height;
} LG_WindowResizeEvent;

/**
 * @brief Widget clicked event
 */
typedef struct {
    LG_WidgetHandle widget;
    int x;
    int y;
} LG_WidgetClickedEvent;

/**
 * @brief Event structure
 */
typedef struct LG_Event {
    LG_EventType type;
    LG_WindowHandle window;
    union {
        LG_MouseButtonEvent mouse_button;
        LG_MouseMoveEvent mouse_move;
        LG_KeyEvent key;
        LG_WindowResizeEvent window_resize;
        LG_WidgetClickedEvent widget_clicked;
    } data;
} LG_Event;

/**
 * @brief Event callback function type
 */
typedef void (*LG_EventCallback)(const LG_Event* event, void* user_data);

/* ========================================================================= */
/*                              API Functions                                */
/* ========================================================================= */

/**
 * @brief Initialize the LightGUI framework
 * 
 * This function must be called before any other LightGUI function.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool LG_Initialize(void);

/**
 * @brief Terminate the LightGUI framework
 * 
 * This function should be called when you're done using LightGUI.
 * It frees all resources allocated by the framework.
 */
void LG_Terminate(void);

/**
 * @brief Create a new window
 * 
 * @param title The window title
 * @param width The window width
 * @param height The window height
 * @param resizable Whether the window is resizable
 * @return A handle to the created window, or NULL if creation failed
 */
LG_WindowHandle LG_CreateWindow(const char* title, int width, int height, bool resizable);

/**
 * @brief Destroy a window
 * 
 * @param window The window to destroy
 */
void LG_DestroyWindow(LG_WindowHandle window);

/**
 * @brief Show a window
 * 
 * @param window The window to show
 */
void LG_ShowWindow(LG_WindowHandle window);

/**
 * @brief Hide a window
 * 
 * @param window The window to hide
 */
void LG_HideWindow(LG_WindowHandle window);

/**
 * @brief Set the window title
 * 
 * @param window The window
 * @param title The new title
 */
void LG_SetWindowTitle(LG_WindowHandle window, const char* title);

/**
 * @brief Create a button widget
 * 
 * @param window The parent window
 * @param text The button text
 * @param x The x position
 * @param y The y position
 * @param width The button width
 * @param height The button height
 * @return A handle to the created button, or NULL if creation failed
 */
LG_WidgetHandle LG_CreateButton(LG_WindowHandle window, const char* text, 
                               int x, int y, int width, int height);

/**
 * @brief Create a label widget
 * 
 * @param window The parent window
 * @param text The label text
 * @param x The x position
 * @param y The y position
 * @param width The label width
 * @param height The label height
 * @return A handle to the created label, or NULL if creation failed
 */
LG_WidgetHandle LG_CreateLabel(LG_WindowHandle window, const char* text, 
                              int x, int y, int width, int height);

/**
 * @brief Create a text field widget
 * 
 * @param window The parent window
 * @param text The initial text
 * @param x The x position
 * @param y The y position
 * @param width The text field width
 * @param height The text field height
 * @return A handle to the created text field, or NULL if creation failed
 */
LG_WidgetHandle LG_CreateTextField(LG_WindowHandle window, const char* text, 
                                  int x, int y, int width, int height);

/**
 * @brief Set a widget's text
 * 
 * @param widget The widget
 * @param text The new text
 */
void LG_SetWidgetText(LG_WidgetHandle widget, const char* text);

/**
 * @brief Get a widget's text
 * 
 * @param widget The widget
 * @param buffer Buffer to store the text
 * @param buffer_size Size of the buffer
 * @return The number of characters copied, or -1 on error
 */
int LG_GetWidgetText(LG_WidgetHandle widget, char* buffer, size_t buffer_size);

/**
 * @brief Set a widget's position
 * 
 * @param widget The widget
 * @param x The new x position
 * @param y The new y position
 */
void LG_SetWidgetPosition(LG_WidgetHandle widget, int x, int y);

/**
 * @brief Set a widget's size
 * 
 * @param widget The widget
 * @param width The new width
 * @param height The new height
 */
void LG_SetWidgetSize(LG_WidgetHandle widget, int width, int height);

/**
 * @brief Set a widget's visibility
 * 
 * @param widget The widget
 * @param visible Whether the widget should be visible
 */
void LG_SetWidgetVisible(LG_WidgetHandle widget, bool visible);

/**
 * @brief Set a widget's enabled state
 * 
 * @param widget The widget
 * @param enabled Whether the widget should be enabled
 */
void LG_SetWidgetEnabled(LG_WidgetHandle widget, bool enabled);

/**
 * @brief Set a widget's background color
 * 
 * @param widget The widget
 * @param color The new background color
 */
void LG_SetWidgetBackgroundColor(LG_WidgetHandle widget, LG_Color color);

/**
 * @brief Set a widget's text color
 * 
 * @param widget The widget
 * @param color The new text color
 */
void LG_SetWidgetTextColor(LG_WidgetHandle widget, LG_Color color);

/**
 * @brief Register an event callback for a window
 * 
 * @param window The window
 * @param callback The callback function
 * @param user_data User data to pass to the callback
 */
void LG_SetWindowEventCallback(LG_WindowHandle window, LG_EventCallback callback, void* user_data);

/**
 * @brief Process pending events
 * 
 * This function processes all pending events and dispatches them to the
 * appropriate callbacks.
 * 
 * @return true if the application should continue running, false if it should quit
 */
bool LG_ProcessEvents(void);

/**
 * @brief Render a window
 * 
 * This function renders the window and all its widgets.
 * 
 * @param window The window to render
 */
void LG_RenderWindow(LG_WindowHandle window);

/**
 * @brief Run the main event loop
 * 
 * This function runs the main event loop until the application is closed.
 */
void LG_Run(void);

/**
 * @brief Create a predefined color
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return The created color
 */
LG_Color LG_CreateColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Predefined colors */
#define LG_COLOR_BLACK       LG_CreateColor(0, 0, 0, 255)
#define LG_COLOR_WHITE       LG_CreateColor(255, 255, 255, 255)
#define LG_COLOR_RED         LG_CreateColor(255, 0, 0, 255)
#define LG_COLOR_GREEN       LG_CreateColor(0, 255, 0, 255)
#define LG_COLOR_BLUE        LG_CreateColor(0, 0, 255, 255)
#define LG_COLOR_YELLOW      LG_CreateColor(255, 255, 0, 255)
#define LG_COLOR_CYAN        LG_CreateColor(0, 255, 255, 255)
#define LG_COLOR_MAGENTA     LG_CreateColor(255, 0, 255, 255)
#define LG_COLOR_TRANSPARENT LG_CreateColor(0, 0, 0, 0)

/* Platform-specific functions (for internal use) */
bool LG_PlatformInitialize(void);
void LG_PlatformTerminate(void);
bool LG_PlatformCreateWindow(LG_WindowHandle window);
void LG_PlatformDestroyWindow(LG_WindowHandle window);
void LG_PlatformShowWindow(LG_WindowHandle window);
void LG_PlatformHideWindow(LG_WindowHandle window);
void LG_PlatformSetWindowTitle(LG_WindowHandle window, const char* title);
bool LG_PlatformCreateWidget(LG_WidgetHandle widget);
void LG_PlatformDestroyWidget(LG_WidgetHandle widget);
void LG_PlatformUpdateWidget(LG_WidgetHandle widget);
bool LG_PlatformProcessEvents(void);
void LG_PlatformRenderWindow(LG_WindowHandle window);

/* Internal helper functions */
void LG_DestroyWidget(LG_WidgetHandle widget);

/**
 * @brief Set an event callback for a window
 * 
 * @param window The window
 * @param callback The callback function
 * @param user_data User data to pass to the callback
 */
void LG_SetEventCallback(LG_WindowHandle window, LG_EventCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTGUI_H */ 