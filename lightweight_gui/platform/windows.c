/**
 * @file windows.c
 * @brief Windows platform implementation for the LightGUI framework
 */

#ifdef _WIN32

#include "../include/lightgui.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h> // For GET_X_LPARAM, GET_Y_LPARAM
#include <commctrl.h> // For common controls

// Link with the required libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")

// Manifest for Common Controls v6
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

/* ========================================================================= */
/*                        Platform-Specific Structures                       */
/* ========================================================================= */

/**
 * @brief Windows-specific window data
 */
typedef struct {
    HWND hwnd;
    HDC hdc;
    HBITMAP bitmap;
    HDC memory_dc;
    bool needs_redraw;  // Flag to indicate if window needs redrawing
} WindowData;

/**
 * @brief Windows-specific widget data
 */
typedef struct {
    HWND hwnd;
    WNDPROC original_proc;
} WidgetData;

/* ========================================================================= */
/*                        Global Variables and Constants                     */
/* ========================================================================= */

static HINSTANCE g_instance = NULL;
static const wchar_t* WINDOW_CLASS_NAME = L"LightGUI_Window";
static const wchar_t* WIDGET_CLASS_NAME = L"LightGUI_Widget";
static ATOM g_window_class = 0;
static ATOM g_widget_class = 0;
static int g_next_widget_id = 1000; // Starting ID for widgets

// Define event types for debugging
const char* EVENT_TYPE_NAMES[] = {
    "UNKNOWN",
    "WINDOW_CLOSE",
    "WINDOW_RESIZE",
    "MOUSE_MOVE",
    "MOUSE_BUTTON",
    "WIDGET_CLICKED",
    "KEY"
};

/* ========================================================================= */
/*                        Helper Functions                                   */
/* ========================================================================= */

/**
 * @brief Convert a UTF-8 string to a wide string
 */
static wchar_t* Utf8ToWide(const char* utf8) {
    if (!utf8) return NULL;
    
    // Get required buffer size
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (size <= 0) return NULL;
    
    // Allocate buffer
    wchar_t* wide = (wchar_t*)malloc(size * sizeof(wchar_t));
    if (!wide) return NULL;
    
    // Convert string
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, size) <= 0) {
        free(wide);
        return NULL;
    }
    
    return wide;
}

/**
 * @brief Convert a wide string to a UTF-8 string
 */
static char* WideToUtf8(const wchar_t* wide) {
    if (!wide) return NULL;
    
    // Get required buffer size
    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return NULL;
    
    // Allocate buffer
    char* utf8 = (char*)malloc(size);
    if (!utf8) return NULL;
    
    // Convert string
    if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, size, NULL, NULL) <= 0) {
        free(utf8);
        return NULL;
    }
    
    return utf8;
}

/**
 * @brief Convert an LG_Color to a COLORREF
 */
static COLORREF ColorToColorRef(LG_Color color) {
    return RGB(color.r, color.g, color.b);
}

/**
 * @brief Find the window handle for a given LG_WindowHandle
 */
static HWND GetWindowHwnd(LG_WindowHandle window) {
    if (!window || !window->platform_data) return NULL;
    return ((WindowData*)window->platform_data)->hwnd;
}

/**
 * @brief Find the LG_WindowHandle for a given HWND
 */
static LG_WindowHandle FindWindowByHwnd(HWND hwnd) {
    // This is a simple implementation that iterates through all windows
    // A more efficient implementation would use a hash table
    extern LG_WindowList g_windows;
    for (size_t i = 0; i < g_windows.count; i++) {
        if (GetWindowHwnd(g_windows.windows[i]) == hwnd) {
            return g_windows.windows[i];
        }
    }
    return NULL;
}

/**
 * @brief Find the LG_WidgetHandle for a given HWND
 */
static LG_WidgetHandle FindWidgetByHwnd(HWND hwnd) {
    // This is a simple implementation that iterates through all windows and widgets
    // A more efficient implementation would use a hash table
    extern LG_WindowList g_windows;
    for (size_t i = 0; i < g_windows.count; i++) {
        LG_WindowHandle window = g_windows.windows[i];
        for (size_t j = 0; j < window->widgets.count; j++) {
            LG_WidgetHandle widget = window->widgets.widgets[j];
            if (widget->platform_data && ((WidgetData*)widget->platform_data)->hwnd == hwnd) {
                return widget;
            }
        }
    }
    return NULL;
}

/**
 * @brief Dispatch an event to a window's callback
 */
static void DispatchEvent(LG_WindowHandle window, LG_Event* event) {
    if (window && window->event_callback) {
        event->window = window;
        
        // Debug output for event type
        if (event->type >= 0 && event->type <= 6) {
            // printf("Event dispatched: type=%s\n", EVENT_TYPE_NAMES[event->type]);
        } else {
            // printf("Event dispatched: type=%d (unknown)\n", event->type);
        }
        
        window->event_callback(event, window->user_data);
    }
}

/* ========================================================================= */
/*                        Window Procedure                                   */
/* ========================================================================= */

/**
 * @brief Window procedure for LightGUI windows
 */
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LG_WindowHandle window = FindWindowByHwnd(hwnd);
    
    switch (msg) {
        case WM_CREATE:
            // Additional initialization if needed
            return 0;
            
        case WM_CLOSE:
            {
                LG_Event event;
                event.type = LG_EVENT_WINDOW_CLOSE;
                DispatchEvent(window, &event);
                return 0;
            }
            
        case WM_SIZE:
            if (window) {
                window->width = LOWORD(lparam);
                window->height = HIWORD(lparam);
                
                LG_Event event;
                event.type = LG_EVENT_WINDOW_RESIZE;
                event.data.window_resize.width = window->width;
                event.data.window_resize.height = window->height;
                DispatchEvent(window, &event);
                
                // Mark window for redraw
                if (window->platform_data) {
                    WindowData* data = (WindowData*)window->platform_data;
                    data->needs_redraw = true;
                }
            }
            return 0;
            
        case WM_MOUSEMOVE:
            {
                LG_Event event;
                event.type = LG_EVENT_MOUSE_MOVE;
                event.data.mouse_move.x = GET_X_LPARAM(lparam);
                event.data.mouse_move.y = GET_Y_LPARAM(lparam);
                
                // Calculate delta (this is simplified)
                static int last_x = 0, last_y = 0;
                event.data.mouse_move.delta_x = event.data.mouse_move.x - last_x;
                event.data.mouse_move.delta_y = event.data.mouse_move.y - last_y;
                last_x = event.data.mouse_move.x;
                last_y = event.data.mouse_move.y;
                
                DispatchEvent(window, &event);
            }
            return 0;
            
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            {
                LG_Event event;
                event.type = LG_EVENT_MOUSE_BUTTON;
                event.data.mouse_button.x = GET_X_LPARAM(lparam);
                event.data.mouse_button.y = GET_Y_LPARAM(lparam);
                
                switch (msg) {
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                        event.data.mouse_button.button = LG_MOUSE_BUTTON_LEFT;
                        break;
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                        event.data.mouse_button.button = LG_MOUSE_BUTTON_RIGHT;
                        break;
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                        event.data.mouse_button.button = LG_MOUSE_BUTTON_MIDDLE;
                        break;
                }
                
                event.data.mouse_button.pressed = (msg == WM_LBUTTONDOWN || 
                                                  msg == WM_RBUTTONDOWN || 
                                                  msg == WM_MBUTTONDOWN);
                
                DispatchEvent(window, &event);
            }
            return 0;
            
        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                LG_Event event;
                event.type = LG_EVENT_KEY;
                event.data.key.key_code = (int)wparam;
                event.data.key.pressed = (msg == WM_KEYDOWN);
                event.data.key.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.data.key.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.data.key.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                
                DispatchEvent(window, &event);
            }
            return 0;
            
        case WM_COMMAND:
            {
                // Handle widget events
                if (lparam != 0) {  // If lparam is non-zero, it's the HWND of the control
                    HWND control_hwnd = (HWND)lparam;
                    LG_WidgetHandle widget = FindWidgetByHwnd(control_hwnd);
                    
                    if (widget) {
                        WORD notification = HIWORD(wparam);
                        
                        switch (notification) {
                            case BN_CLICKED:
                                if (widget->type == LG_WIDGET_BUTTON) {
                                    LG_Event event;
                                    event.type = LG_EVENT_WIDGET_CLICKED;
                                    event.data.widget_clicked.widget = widget;
                                    event.data.widget_clicked.x = widget->rect.x;
                                    event.data.widget_clicked.y = widget->rect.y;
                                    
                                    DispatchEvent(window, &event);
                                }
                                break;
                        }
                    }
                }
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                if (window && window->platform_data) {
                    WindowData* data = (WindowData*)window->platform_data;
                    
                    // Blit the memory DC to the window DC
                    BitBlt(hdc, 0, 0, window->width, window->height, 
                           data->memory_dc, 0, 0, SRCCOPY);
                    
                    // Reset the needs_redraw flag since we just painted
                    data->needs_redraw = false;
                }
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            
        case WM_ERASEBKGND:
            // Return 1 to indicate that we've handled this message
            // This prevents flickering
            return 1;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/**
 * @brief Subclass procedure for widgets
 */
static LRESULT CALLBACK WidgetSubclassProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                          UINT_PTR subclass_id, DWORD_PTR ref_data) {
    LG_WidgetHandle widget = (LG_WidgetHandle)ref_data;
    
    switch (msg) {
        // Handle widget-specific messages here
    }
    
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

/* ========================================================================= */
/*                        Platform API Implementation                        */
/* ========================================================================= */

bool LG_PlatformInitialize(void) {
    // Initialize common controls
    // Use a simpler approach that's more likely to work
    InitCommonControls();
    
    // Get instance handle
    g_instance = GetModuleHandle(NULL);
    if (!g_instance) {
        fprintf(stderr, "LightGUI: Failed to get module handle\n");
        return false;
    }
    
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = g_instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    g_window_class = RegisterClassExW(&wc);
    if (!g_window_class) {
        fprintf(stderr, "LightGUI: Failed to register window class\n");
        return false;
    }
    
    return true;
}

void LG_PlatformTerminate(void) {
    if (g_window_class) {
        UnregisterClassW(WINDOW_CLASS_NAME, g_instance);
        g_window_class = 0;
    }
}

bool LG_PlatformCreateWindow(LG_WindowHandle window) {
    if (!window) return false;
    
    // Allocate platform-specific data
    WindowData* data = (WindowData*)malloc(sizeof(WindowData));
    if (!data) {
        fprintf(stderr, "LightGUI: Failed to allocate window data\n");
        return false;
    }
    
    // Initialize the data structure
    memset(data, 0, sizeof(WindowData));
    data->needs_redraw = true;  // Initial draw needed
    
    // Convert title to wide string
    wchar_t* title_wide = Utf8ToWide(window->title);
    if (!title_wide) {
        fprintf(stderr, "LightGUI: Failed to convert window title\n");
        free(data);
        return false;
    }
    
    // Create window
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!window->resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    
    // Calculate the actual window size to accommodate the client area
    RECT rect = {0, 0, window->width, window->height};
    AdjustWindowRect(&rect, style, FALSE);
    int actual_width = rect.right - rect.left;
    int actual_height = rect.bottom - rect.top;
    
    data->hwnd = CreateWindowExW(
        0,                          // Extended style
        WINDOW_CLASS_NAME,          // Class name
        title_wide,                 // Window title
        style,                      // Style
        CW_USEDEFAULT,              // X position
        CW_USEDEFAULT,              // Y position
        actual_width,               // Width
        actual_height,              // Height
        NULL,                       // Parent window
        NULL,                       // Menu
        g_instance,                 // Instance
        NULL                        // Additional data
    );
    
    free(title_wide);
    
    if (!data->hwnd) {
        fprintf(stderr, "LightGUI: Failed to create window\n");
        free(data);
        return false;
    }
    
    // Get device context
    data->hdc = GetDC(data->hwnd);
    if (!data->hdc) {
        fprintf(stderr, "LightGUI: Failed to get device context\n");
        DestroyWindow(data->hwnd);
        free(data);
        return false;
    }
    
    // Create memory DC and bitmap for double buffering
    data->memory_dc = CreateCompatibleDC(data->hdc);
    if (!data->memory_dc) {
        fprintf(stderr, "LightGUI: Failed to create memory DC\n");
        ReleaseDC(data->hwnd, data->hdc);
        DestroyWindow(data->hwnd);
        free(data);
        return false;
    }
    
    data->bitmap = CreateCompatibleBitmap(data->hdc, window->width, window->height);
    if (!data->bitmap) {
        fprintf(stderr, "LightGUI: Failed to create bitmap\n");
        DeleteDC(data->memory_dc);
        ReleaseDC(data->hwnd, data->hdc);
        DestroyWindow(data->hwnd);
        free(data);
        return false;
    }
    
    SelectObject(data->memory_dc, data->bitmap);
    
    // Clear the background
    RECT client_rect;
    GetClientRect(data->hwnd, &client_rect);
    FillRect(data->memory_dc, &client_rect, (HBRUSH)(COLOR_WINDOW + 1));
    
    // Store platform data in window
    window->platform_data = data;
    
    return true;
}

void LG_PlatformDestroyWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    
    // Clean up resources
    DeleteObject(data->bitmap);
    DeleteDC(data->memory_dc);
    ReleaseDC(data->hwnd, data->hdc);
    DestroyWindow(data->hwnd);
    
    free(data);
    window->platform_data = NULL;
}

void LG_PlatformShowWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    ShowWindow(data->hwnd, SW_SHOW);
    UpdateWindow(data->hwnd);
}

void LG_PlatformHideWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    ShowWindow(data->hwnd, SW_HIDE);
}

void LG_PlatformSetWindowTitle(LG_WindowHandle window, const char* title) {
    if (!window || !window->platform_data || !title) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    
    // Convert title to wide string
    wchar_t* title_wide = Utf8ToWide(title);
    if (!title_wide) return;
    
    SetWindowTextW(data->hwnd, title_wide);
    free(title_wide);
}

bool LG_PlatformCreateWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->window || !widget->window->platform_data) return false;
    
    WindowData* window_data = (WindowData*)widget->window->platform_data;
    
    // Allocate platform-specific data
    WidgetData* data = (WidgetData*)malloc(sizeof(WidgetData));
    if (!data) {
        fprintf(stderr, "LightGUI: Failed to allocate widget data\n");
        return false;
    }
    
    // Convert text to wide string
    wchar_t* text_wide = Utf8ToWide(widget->text);
    if (!text_wide) {
        fprintf(stderr, "LightGUI: Failed to convert widget text\n");
        free(data);
        return false;
    }
    
    // Assign a unique ID to the widget if it doesn't have one
    if (widget->id == 0) {
        widget->id = g_next_widget_id++;
    }
    
    // Create widget based on type
    HWND hwnd = NULL;
    DWORD style = WS_CHILD | WS_VISIBLE;
    
    switch (widget->type) {
        case LG_WIDGET_BUTTON:
            hwnd = CreateWindowW(
                L"BUTTON",                  // Class name
                text_wide,                  // Button text
                style | BS_PUSHBUTTON,      // Style
                widget->rect.x,             // X position
                widget->rect.y,             // Y position
                widget->rect.width,         // Width
                widget->rect.height,        // Height
                window_data->hwnd,          // Parent window
                (HMENU)(INT_PTR)widget->id, // Menu (used as control ID)
                g_instance,                 // Instance
                NULL                        // Additional data
            );
            break;
            
        case LG_WIDGET_LABEL:
            hwnd = CreateWindowW(
                L"STATIC",                  // Class name
                text_wide,                  // Label text
                style | SS_LEFT,            // Style
                widget->rect.x,             // X position
                widget->rect.y,             // Y position
                widget->rect.width,         // Width
                widget->rect.height,        // Height
                window_data->hwnd,          // Parent window
                (HMENU)(INT_PTR)widget->id, // Menu (used as control ID)
                g_instance,                 // Instance
                NULL                        // Additional data
            );
            break;
            
        case LG_WIDGET_TEXTFIELD:
            hwnd = CreateWindowW(
                L"EDIT",                    // Class name
                text_wide,                  // Text
                style | WS_BORDER | ES_AUTOHSCROLL, // Style
                widget->rect.x,             // X position
                widget->rect.y,             // Y position
                widget->rect.width,         // Width
                widget->rect.height,        // Height
                window_data->hwnd,          // Parent window
                (HMENU)(INT_PTR)widget->id, // Menu (used as control ID)
                g_instance,                 // Instance
                NULL                        // Additional data
            );
            break;
            
        default:
            fprintf(stderr, "LightGUI: Unsupported widget type\n");
            free(text_wide);
            free(data);
            return false;
    }
    
    free(text_wide);
    
    if (!hwnd) {
        fprintf(stderr, "LightGUI: Failed to create widget\n");
        free(data);
        return false;
    }
    
    // Set widget colors
    if (widget->type != LG_WIDGET_TEXTFIELD) {
        SetBkColor(window_data->memory_dc, ColorToColorRef(widget->bg_color));
        SetTextColor(window_data->memory_dc, ColorToColorRef(widget->text_color));
    }
    
    // Store platform data in widget
    data->hwnd = hwnd;
    data->original_proc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)widget);
    
    widget->platform_data = data;
    
    return true;
}

void LG_PlatformDestroyWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->platform_data) return;
    
    WidgetData* data = (WidgetData*)widget->platform_data;
    
    // Destroy widget
    DestroyWindow(data->hwnd);
    
    free(data);
    widget->platform_data = NULL;
}

void LG_PlatformUpdateWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->platform_data) return;
    
    WidgetData* data = (WidgetData*)widget->platform_data;
    
    // Update widget properties
    if (widget->text) {
        wchar_t* text_wide = Utf8ToWide(widget->text);
        if (text_wide) {
            SetWindowTextW(data->hwnd, text_wide);
            free(text_wide);
        }
    }
    
    // Update position and size
    SetWindowPos(data->hwnd, NULL, 
                widget->rect.x, widget->rect.y, 
                widget->rect.width, widget->rect.height, 
                SWP_NOZORDER);
    
    // Update visibility
    ShowWindow(data->hwnd, widget->visible ? SW_SHOW : SW_HIDE);
    
    // Update enabled state
    EnableWindow(data->hwnd, widget->enabled);
    
    // Update colors (for some controls)
    if (widget->type != LG_WIDGET_TEXTFIELD) {
        WindowData* window_data = (WindowData*)widget->window->platform_data;
        SetBkColor(window_data->memory_dc, ColorToColorRef(widget->bg_color));
        SetTextColor(window_data->memory_dc, ColorToColorRef(widget->text_color));
    }
    
    // Force redraw of the widget only
    InvalidateRect(data->hwnd, NULL, TRUE);
}

bool LG_PlatformProcessEvents(void) {
    MSG msg;
    
    // Process all pending messages
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return true;
}

void LG_PlatformRenderWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    
    // Only redraw if needed
    if (data->needs_redraw) {
        // Clear background
        RECT rect;
        GetClientRect(data->hwnd, &rect);
        FillRect(data->memory_dc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
        
        // Blit memory DC to window DC
        HDC hdc = GetDC(data->hwnd);
        BitBlt(hdc, 0, 0, window->width, window->height, 
               data->memory_dc, 0, 0, SRCCOPY);
        ReleaseDC(data->hwnd, hdc);
        
        // Force a redraw
        InvalidateRect(data->hwnd, NULL, FALSE);
        UpdateWindow(data->hwnd);
        
        // Reset the needs_redraw flag
        data->needs_redraw = false;
    }
}

#endif /* _WIN32 */ 