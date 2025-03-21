/**
 * @file simple_paint.c
 * @brief Simple drawing application using the LightGUI framework
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
// GDI+ headers
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define CANVAS_WIDTH 700
#define CANVAS_HEIGHT 500
#define MAX_PATH_POINTS 1000
#define COLOR_PICKER_SIZE 20
#define NUM_COLORS 8

// Widget handles
LG_WindowHandle window;
LG_WidgetHandle canvas;
LG_WidgetHandle color_buttons[NUM_COLORS];
LG_WidgetHandle brush_size_label;
LG_WidgetHandle brush_size_plus;
LG_WidgetHandle brush_size_minus;
LG_WidgetHandle clear_button;
LG_WidgetHandle status_label;

// Drawing state
typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point points[MAX_PATH_POINTS];
    int num_points;
    int color_index;
    int brush_size;
} DrawPath;

#define MAX_PATHS 100
DrawPath paths[MAX_PATHS];
int path_count = 0;
bool is_drawing = false;
int current_point = 0;
int current_color = 0;
int current_brush_size = 5;

// Colors
LG_Color colors[NUM_COLORS] = {
    {0, 0, 0, 255},       // Black
    {255, 0, 0, 255},     // Red
    {0, 255, 0, 255},     // Green
    {0, 0, 255, 255},     // Blue
    {255, 255, 0, 255},   // Yellow
    {255, 0, 255, 255},   // Magenta
    {0, 255, 255, 255},   // Cyan
    {255, 255, 255, 255}  // White
};

#ifdef _WIN32
// GDI+ variables
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
HWND canvas_hwnd = NULL;
HDC canvas_hdc = NULL;
#endif

/**
 * @brief Initialize drawing system
 */
bool init_drawing() {
#ifdef _WIN32
    // Initialize GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Get canvas HWND
    canvas_hwnd = (HWND)LG_GetCanvasContext(canvas);
    if (!canvas_hwnd) {
        fprintf(stderr, "Failed to get canvas HWND\n");
        return false;
    }
    
    // Get canvas HDC
    canvas_hdc = GetDC(canvas_hwnd);
    if (!canvas_hdc) {
        fprintf(stderr, "Failed to get canvas HDC\n");
        return false;
    }
#endif
    return true;
}

/**
 * @brief Clean up drawing system
 */
void cleanup_drawing() {
#ifdef _WIN32
    if (canvas_hdc && canvas_hwnd) {
        ReleaseDC(canvas_hwnd, canvas_hdc);
    }
    GdiplusShutdown(gdiplusToken);
#endif
}

/**
 * @brief Draw a line
 */
void draw_line(int x1, int y1, int x2, int y2, LG_Color color, int width) {
#ifdef _WIN32
    if (!canvas_hdc) return;
    
    Graphics graphics(canvas_hdc);
    Pen pen(Color(color.a, color.r, color.g, color.b), (REAL)width);
    graphics.DrawLine(&pen, x1, y1, x2, y2);
#else
    // Non-Windows implementation would go here
    // For simplicity, we're focusing on Windows for now
#endif
}

/**
 * @brief Draw a circle
 */
void draw_circle(int x, int y, int radius, LG_Color color, bool filled) {
#ifdef _WIN32
    if (!canvas_hdc) return;
    
    Graphics graphics(canvas_hdc);
    if (filled) {
        SolidBrush brush(Color(color.a, color.r, color.g, color.b));
        graphics.FillEllipse(&brush, x - radius, y - radius, radius * 2, radius * 2);
    } else {
        Pen pen(Color(color.a, color.r, color.g, color.b), 1);
        graphics.DrawEllipse(&pen, x - radius, y - radius, radius * 2, radius * 2);
    }
#else
    // Non-Windows implementation
#endif
}

/**
 * @brief Clear canvas
 */
void clear_canvas() {
#ifdef _WIN32
    if (!canvas_hdc) return;
    
    Graphics graphics(canvas_hdc);
    graphics.Clear(Color(255, 255, 255, 255)); // White background
#else
    // Non-Windows implementation
#endif
    
    // Reset paths
    path_count = 0;
    current_point = 0;
    
    // Update status
    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Canvas cleared");
    LG_SetWidgetText(status_label, status_text);
}

/**
 * @brief Redraw all paths
 */
void redraw_all_paths() {
    clear_canvas();
    
    for (int i = 0; i < path_count; i++) {
        DrawPath* path = &paths[i];
        for (int j = 1; j < path->num_points; j++) {
            draw_line(
                path->points[j-1].x, path->points[j-1].y,
                path->points[j].x, path->points[j].y,
                colors[path->color_index],
                path->brush_size
            );
        }
    }
    
#ifdef _WIN32
    // Force canvas to update
    if (canvas_hwnd) {
        InvalidateRect(canvas_hwnd, NULL, FALSE);
    }
#endif
}

/**
 * @brief Start a new drawing path
 */
void start_path(int x, int y) {
    if (path_count >= MAX_PATHS) {
        // If we've reached the maximum number of paths, reset
        path_count = 0;
    }
    
    DrawPath* path = &paths[path_count];
    path->num_points = 0;
    path->color_index = current_color;
    path->brush_size = current_brush_size;
    
    // Add first point
    path->points[path->num_points].x = x;
    path->points[path->num_points].y = y;
    path->num_points++;
    
    is_drawing = true;
    current_point = path->num_points;
}

/**
 * @brief Add a point to the current path
 */
void add_to_path(int x, int y) {
    if (!is_drawing || path_count >= MAX_PATHS) return;
    
    DrawPath* path = &paths[path_count];
    
    if (path->num_points >= MAX_PATH_POINTS) {
        // If we've reached the maximum number of points, finish this path
        is_drawing = false;
        path_count++;
        return;
    }
    
    // Calculate the previous point
    int prev_x = path->points[path->num_points - 1].x;
    int prev_y = path->points[path->num_points - 1].y;
    
    // Add new point
    path->points[path->num_points].x = x;
    path->points[path->num_points].y = y;
    path->num_points++;
    
    // Draw line from previous point to new point
    draw_line(prev_x, prev_y, x, y, colors[path->color_index], path->brush_size);
    
#ifdef _WIN32
    // Force canvas to update
    if (canvas_hwnd) {
        InvalidateRect(canvas_hwnd, NULL, FALSE);
    }
#endif
    
    current_point = path->num_points;
}

/**
 * @brief End the current drawing path
 */
void end_path() {
    if (!is_drawing) return;
    
    is_drawing = false;
    path_count++;
    
    // Update status
    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Path completed: %d points", 
             paths[path_count - 1].num_points);
    LG_SetWidgetText(status_label, status_text);
}

/**
 * @brief Set current drawing color
 */
void set_color(int color_index) {
    if (color_index < 0 || color_index >= NUM_COLORS) return;
    
    current_color = color_index;
    
    // Update status
    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Color changed");
    LG_SetWidgetText(status_label, status_text);
}

/**
 * @brief Change brush size
 */
void change_brush_size(int delta) {
    current_brush_size += delta;
    
    // Clamp brush size
    if (current_brush_size < 1) current_brush_size = 1;
    if (current_brush_size > 50) current_brush_size = 50;
    
    // Update brush size label
    char brush_text[16];
    snprintf(brush_text, sizeof(brush_text), "Size: %d", current_brush_size);
    LG_SetWidgetText(brush_size_label, brush_text);
    
    // Update status
    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Brush size: %d", current_brush_size);
    LG_SetWidgetText(status_label, status_text);
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
                
                // Check color buttons
                for (int i = 0; i < NUM_COLORS; i++) {
                    if (widget == color_buttons[i]) {
                        set_color(i);
                        return;
                    }
                }
                
                // Check other buttons
                if (widget == brush_size_plus) {
                    change_brush_size(1);
                }
                else if (widget == brush_size_minus) {
                    change_brush_size(-1);
                }
                else if (widget == clear_button) {
                    clear_canvas();
                }
            }
            break;
            
        case LG_EVENT_MOUSE_BUTTON:
            {
                if (event->data.mouse_button.button == LG_MOUSE_BUTTON_LEFT) {
                    // Check if the event is inside the canvas
                    int x = event->data.mouse_button.x;
                    int y = event->data.mouse_button.y;
                    
                    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
                        if (event->data.mouse_button.pressed) {
                            start_path(x, y);
                        } else {
                            end_path();
                        }
                    }
                }
            }
            break;
            
        case LG_EVENT_MOUSE_MOVE:
            {
                // Check if we're drawing and the mouse is inside the canvas
                int x = event->data.mouse_move.x;
                int y = event->data.mouse_move.y;
                
                if (is_drawing && x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
                    if (event->data.mouse_move.button_pressed[LG_MOUSE_BUTTON_LEFT]) {
                        add_to_path(x, y);
                    }
                }
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
    window = LG_CreateWindow("Simple Paint", WINDOW_WIDTH, WINDOW_HEIGHT, false);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create canvas
    canvas = LG_CreateCanvas(window, 20, 20, CANVAS_WIDTH, CANVAS_HEIGHT);
    if (!canvas) {
        fprintf(stderr, "Failed to create canvas\n");
        LG_DestroyWindow(window);
        LG_Terminate();
        return 1;
    }
    
    // Initialize drawing system
    if (!init_drawing()) {
        fprintf(stderr, "Failed to initialize drawing system\n");
        LG_DestroyWindow(window);
        LG_Terminate();
        return 1;
    }
    
    // Create color buttons
    int color_button_x = CANVAS_WIDTH + 40;
    int color_button_y = 20;
    
    for (int i = 0; i < NUM_COLORS; i++) {
        color_buttons[i] = LG_CreateButton(window, "", color_button_x, color_button_y, COLOR_PICKER_SIZE, COLOR_PICKER_SIZE);
        LG_SetWidgetBackgroundColor(color_buttons[i], colors[i]);
        
        // Move to next position
        color_button_y += COLOR_PICKER_SIZE + 10;
    }
    
    // Create brush size controls
    brush_size_label = LG_CreateLabel(window, "Size: 5", color_button_x, color_button_y, 70, 25);
    color_button_y += 30;
    
    brush_size_minus = LG_CreateButton(window, "-", color_button_x, color_button_y, 30, 30);
    brush_size_plus = LG_CreateButton(window, "+", color_button_x + 40, color_button_y, 30, 30);
    color_button_y += 50;
    
    // Create clear button
    clear_button = LG_CreateButton(window, "Clear", color_button_x, color_button_y, 70, 30);
    color_button_y += 50;
    
    // Create status label
    status_label = LG_CreateLabel(window, "Ready to draw", 20, CANVAS_HEIGHT + 30, WINDOW_WIDTH - 40, 25);
    
    // Clear canvas
    clear_canvas();
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    cleanup_drawing();
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 