/**
 * @file lightgui.c
 * @brief Main implementation file for the LightGUI framework
 */

#include "../include/lightgui.h"
#include "lightgui_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Platform-specific includes */
#ifdef _WIN32
#include <windows.h> /* For Sleep() */
#else
#include <unistd.h> /* For usleep() */
#endif

/* Define a cross-platform strdup function */
#ifdef _WIN32
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

/* Define a cross-platform sleep function (milliseconds) */
#ifdef _WIN32
#define SLEEP_MS(ms) Sleep(ms)
#else
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

/* ========================================================================= */
/*                           Global State Variables                          */
/* ========================================================================= */

static bool g_initialized = false;
static bool g_event_loop_running = false;
LG_WindowList g_windows = {NULL, 0, 0};  /* Define the global window list */

/* ========================================================================= */
/*                           Framework Initialization                        */
/* ========================================================================= */

bool LG_Initialize(void) {
    if (g_initialized) {
        fprintf(stderr, "LightGUI: Already initialized\n");
        return true;
    }

    // Initialize platform-specific backend
    if (!LG_PlatformInitialize()) {
        fprintf(stderr, "LightGUI: Failed to initialize platform backend\n");
        return false;
    }

    // Initialize window list
    g_windows.capacity = 10;
    g_windows.windows = (LG_WindowHandle*)malloc(
        g_windows.capacity * sizeof(LG_WindowHandle));
    
    if (!g_windows.windows) {
        fprintf(stderr, "LightGUI: Failed to allocate window list\n");
        LG_PlatformTerminate();
        return false;
    }

    g_initialized = true;
    return true;
}

void LG_Terminate(void) {
    if (!g_initialized) {
        fprintf(stderr, "LightGUI: Not initialized\n");
        return;
    }

    // Destroy all windows
    for (size_t i = 0; i < g_windows.count; i++) {
        LG_DestroyWindow(g_windows.windows[i]);
    }

    // Free window list
    free(g_windows.windows);
    g_windows.windows = NULL;
    g_windows.count = 0;
    g_windows.capacity = 0;

    // Terminate platform-specific backend
    LG_PlatformTerminate();

    g_initialized = false;
}

/* ========================================================================= */
/*                              Window Management                            */
/* ========================================================================= */

LG_WindowHandle LG_CreateWindow(const char* title, int width, int height, bool resizable) {
    if (!g_initialized) {
        fprintf(stderr, "LightGUI: Not initialized\n");
        return NULL;
    }

    // Allocate window structure
    struct LG_Window* window = (struct LG_Window*)malloc(sizeof(struct LG_Window));
    if (!window) {
        fprintf(stderr, "LightGUI: Failed to allocate window\n");
        return NULL;
    }

    // Initialize window structure
    memset(window, 0, sizeof(struct LG_Window));
    window->width = width;
    window->height = height;
    window->visible = false;
    window->resizable = resizable;
    
    // Copy title
    window->title = STRDUP(title);
    if (!window->title) {
        fprintf(stderr, "LightGUI: Failed to allocate window title\n");
        free(window);
        return NULL;
    }

    // Initialize widget list
    window->widgets.capacity = 20;
    window->widgets.widgets = (LG_WidgetHandle*)malloc(
        window->widgets.capacity * sizeof(LG_WidgetHandle));
    
    if (!window->widgets.widgets) {
        fprintf(stderr, "LightGUI: Failed to allocate widget list\n");
        free(window->title);
        free(window);
        return NULL;
    }

    // Create platform-specific window
    if (!LG_PlatformCreateWindow(window)) {
        fprintf(stderr, "LightGUI: Failed to create platform window\n");
        free(window->widgets.widgets);
        free(window->title);
        free(window);
        return NULL;
    }

    // Add window to global list
    if (g_windows.count >= g_windows.capacity) {
        size_t new_capacity = g_windows.capacity * 2;
        LG_WindowHandle* new_windows = (LG_WindowHandle*)realloc(
            g_windows.windows, new_capacity * sizeof(LG_WindowHandle));
        
        if (!new_windows) {
            fprintf(stderr, "LightGUI: Failed to resize window list\n");
            LG_PlatformDestroyWindow(window);
            free(window->widgets.widgets);
            free(window->title);
            free(window);
            return NULL;
        }

        g_windows.windows = new_windows;
        g_windows.capacity = new_capacity;
    }

    g_windows.windows[g_windows.count++] = window;

    return window;
}

void LG_DestroyWindow(LG_WindowHandle window) {
    if (!g_initialized || !window) {
        return;
    }

    // Destroy all widgets
    for (size_t i = 0; i < window->widgets.count; i++) {
        LG_DestroyWidget(window->widgets.widgets[i]);
    }

    // Free widget list
    free(window->widgets.widgets);

    // Destroy platform-specific window
    LG_PlatformDestroyWindow(window);

    // Remove window from global list
    for (size_t i = 0; i < g_windows.count; i++) {
        if (g_windows.windows[i] == window) {
            // Move last window to this position
            g_windows.windows[i] = g_windows.windows[--g_windows.count];
            break;
        }
    }

    // Free window resources
    free(window->title);
    free(window);
}

void LG_ShowWindow(LG_WindowHandle window) {
    if (!g_initialized || !window) {
        return;
    }

    window->visible = true;
    LG_PlatformShowWindow(window);
}

void LG_HideWindow(LG_WindowHandle window) {
    if (!g_initialized || !window) {
        return;
    }

    window->visible = false;
    LG_PlatformHideWindow(window);
}

void LG_SetWindowTitle(LG_WindowHandle window, const char* title) {
    if (!g_initialized || !window || !title) {
        return;
    }

    // Free old title and set new one
    free(window->title);
    window->title = STRDUP(title);

    // Update platform window
    LG_PlatformSetWindowTitle(window, title);
}

/* ========================================================================= */
/*                              Widget Management                            */
/* ========================================================================= */

void AddWidgetToWindow(LG_WindowHandle window, LG_WidgetHandle widget) {
    // Resize widget list if necessary
    if (window->widgets.count >= window->widgets.capacity) {
        size_t new_capacity = window->widgets.capacity * 2;
        LG_WidgetHandle* new_widgets = (LG_WidgetHandle*)realloc(
            window->widgets.widgets, new_capacity * sizeof(LG_WidgetHandle));
        
        if (!new_widgets) {
            fprintf(stderr, "LightGUI: Failed to resize widget list\n");
            return;
        }

        window->widgets.widgets = new_widgets;
        window->widgets.capacity = new_capacity;
    }

    // Add widget to list
    window->widgets.widgets[window->widgets.count++] = widget;
}

LG_WidgetHandle LG_CreateButton(LG_WindowHandle window, const char* text, 
                               int x, int y, int width, int height) {
    if (!g_initialized || !window || !text) {
        return NULL;
    }

    // Allocate widget structure
    struct LG_Widget* widget = (struct LG_Widget*)malloc(sizeof(struct LG_Widget));
    if (!widget) {
        fprintf(stderr, "LightGUI: Failed to allocate button widget\n");
        return NULL;
    }

    // Initialize widget structure
    memset(widget, 0, sizeof(struct LG_Widget));
    widget->type = LG_WIDGET_BUTTON;
    widget->window = window;
    widget->rect.x = x;
    widget->rect.y = y;
    widget->rect.width = width;
    widget->rect.height = height;
    widget->visible = true;
    widget->enabled = true;
    widget->bg_color = LG_COLOR_WHITE;
    widget->text_color = LG_COLOR_BLACK;
    
    // Copy text
    widget->text = STRDUP(text);
    if (!widget->text) {
        fprintf(stderr, "LightGUI: Failed to allocate button text\n");
        free(widget);
        return NULL;
    }

    // Create platform-specific widget
    if (!LG_PlatformCreateWidget(widget)) {
        fprintf(stderr, "LightGUI: Failed to create platform button\n");
        free(widget->text);
        free(widget);
        return NULL;
    }

    // Add widget to window
    AddWidgetToWindow(window, widget);

    return widget;
}

LG_WidgetHandle LG_CreateLabel(LG_WindowHandle window, const char* text, 
                              int x, int y, int width, int height) {
    if (!g_initialized || !window || !text) {
        return NULL;
    }

    // Allocate widget structure
    struct LG_Widget* widget = (struct LG_Widget*)malloc(sizeof(struct LG_Widget));
    if (!widget) {
        fprintf(stderr, "LightGUI: Failed to allocate label widget\n");
        return NULL;
    }

    // Initialize widget structure
    memset(widget, 0, sizeof(struct LG_Widget));
    widget->type = LG_WIDGET_LABEL;
    widget->window = window;
    widget->rect.x = x;
    widget->rect.y = y;
    widget->rect.width = width;
    widget->rect.height = height;
    widget->visible = true;
    widget->enabled = true;
    widget->bg_color = LG_COLOR_TRANSPARENT;
    widget->text_color = LG_COLOR_BLACK;
    
    // Copy text
    widget->text = STRDUP(text);
    if (!widget->text) {
        fprintf(stderr, "LightGUI: Failed to allocate label text\n");
        free(widget);
        return NULL;
    }

    // Create platform-specific widget
    if (!LG_PlatformCreateWidget(widget)) {
        fprintf(stderr, "LightGUI: Failed to create platform label\n");
        free(widget->text);
        free(widget);
        return NULL;
    }

    // Add widget to window
    AddWidgetToWindow(window, widget);

    return widget;
}

LG_WidgetHandle LG_CreateTextField(LG_WindowHandle window, const char* text, 
                                  int x, int y, int width, int height) {
    if (!g_initialized || !window) {
        return NULL;
    }

    // Allocate widget structure
    struct LG_Widget* widget = (struct LG_Widget*)malloc(sizeof(struct LG_Widget));
    if (!widget) {
        fprintf(stderr, "LightGUI: Failed to allocate text field widget\n");
        return NULL;
    }

    // Initialize widget structure
    memset(widget, 0, sizeof(struct LG_Widget));
    widget->type = LG_WIDGET_TEXTFIELD;
    widget->window = window;
    widget->rect.x = x;
    widget->rect.y = y;
    widget->rect.width = width;
    widget->rect.height = height;
    widget->visible = true;
    widget->enabled = true;
    widget->bg_color = LG_COLOR_WHITE;
    widget->text_color = LG_COLOR_BLACK;
    
    // Copy text
    widget->text = STRDUP(text ? text : "");
    if (!widget->text) {
        fprintf(stderr, "LightGUI: Failed to allocate text field text\n");
        free(widget);
        return NULL;
    }

    // Create platform-specific widget
    if (!LG_PlatformCreateWidget(widget)) {
        fprintf(stderr, "LightGUI: Failed to create platform text field\n");
        free(widget->text);
        free(widget);
        return NULL;
    }

    // Add widget to window
    AddWidgetToWindow(window, widget);

    return widget;
}

void LG_DestroyWidget(LG_WidgetHandle widget) {
    if (!g_initialized || !widget) {
        return;
    }

    // Destroy platform-specific widget
    LG_PlatformDestroyWidget(widget);

    // Remove widget from window
    LG_WindowHandle window = widget->window;
    for (size_t i = 0; i < window->widgets.count; i++) {
        if (window->widgets.widgets[i] == widget) {
            // Move last widget to this position
            window->widgets.widgets[i] = window->widgets.widgets[--window->widgets.count];
            break;
        }
    }

    // Free widget resources
    free(widget->text);
    free(widget);
}

void LG_SetWidgetText(LG_WidgetHandle widget, const char* text) {
    if (!g_initialized || !widget || !text) {
        return;
    }

    // Free old text and set new one
    free(widget->text);
    widget->text = STRDUP(text);

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

int LG_GetWidgetText(LG_WidgetHandle widget, char* buffer, size_t buffer_size) {
    if (!g_initialized || !widget || !buffer || buffer_size == 0) {
        return -1;
    }

    if (!widget->text) {
        buffer[0] = '\0';
        return 0;
    }

    size_t text_len = strlen(widget->text);
    size_t copy_len = text_len < buffer_size - 1 ? text_len : buffer_size - 1;
    
    memcpy(buffer, widget->text, copy_len);
    buffer[copy_len] = '\0';
    
    return (int)copy_len;
}

void LG_SetWidgetPosition(LG_WidgetHandle widget, int x, int y) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->rect.x = x;
    widget->rect.y = y;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

void LG_SetWidgetSize(LG_WidgetHandle widget, int width, int height) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->rect.width = width;
    widget->rect.height = height;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

void LG_SetWidgetVisible(LG_WidgetHandle widget, bool visible) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->visible = visible;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

void LG_SetWidgetEnabled(LG_WidgetHandle widget, bool enabled) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->enabled = enabled;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

void LG_SetWidgetBackgroundColor(LG_WidgetHandle widget, LG_Color color) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->bg_color = color;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

void LG_SetWidgetTextColor(LG_WidgetHandle widget, LG_Color color) {
    if (!g_initialized || !widget) {
        return;
    }

    widget->text_color = color;

    // Update platform widget
    LG_PlatformUpdateWidget(widget);
}

/* ========================================================================= */
/*                              Event Handling                               */
/* ========================================================================= */

void LG_SetEventCallback(LG_WindowHandle window, LG_EventCallback callback, void* user_data) {
    if (!g_initialized || !window) {
        return;
    }

    window->event_callback = callback;
    window->user_data = user_data;
}

bool LG_ProcessEvents(void) {
    if (!g_initialized) {
        return false;
    }

    return LG_PlatformProcessEvents();
}

void LG_RenderWindow(LG_WindowHandle window) {
    if (!g_initialized || !window || !window->visible) {
        return;
    }

    LG_PlatformRenderWindow(window);
}

void LG_Run(void) {
    bool running = true;
    
    while (running) {
        // Process platform events
        running = LG_PlatformProcessEvents();
        
        // Render all windows
        for (size_t i = 0; i < g_windows.count; i++) {
            LG_PlatformRenderWindow(g_windows.windows[i]);
        }
        
        // Add a small sleep to prevent excessive CPU usage
        // This helps reduce flickering and event spam
        SLEEP_MS(10);  // 10ms delay
    }
}

void LG_QuitEventLoop(void) {
    g_event_loop_running = false;
}

/* ========================================================================= */
/*                              Utility Functions                            */
/* ========================================================================= */

LG_Color LG_CreateColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    LG_Color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
} 