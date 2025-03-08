/**
 * @file linux.c
 * @brief Linux platform implementation for the LightGUI framework using X11
 */

#ifdef __linux__

#include "../src/lightgui_internal.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ========================================================================= */
/*                        Platform-Specific Structures                       */
/* ========================================================================= */

/**
 * @brief X11-specific window data
 */
typedef struct {
    Window window;
    GC gc;
    Pixmap buffer;
    XFontStruct* font;
} WindowData;

/**
 * @brief X11-specific widget data
 */
typedef struct {
    Window window;
    int type;  // Internal widget type
} WidgetData;

/* ========================================================================= */
/*                        Global Variables and Constants                     */
/* ========================================================================= */

static Display* g_display = NULL;
static int g_screen = 0;
static Atom g_wm_delete_window = 0;
static XFontStruct* g_default_font = NULL;

/* ========================================================================= */
/*                        Helper Functions                                   */
/* ========================================================================= */

/**
 * @brief Convert an LG_Color to an X11 color
 */
static unsigned long ColorToX11Color(LG_Color color) {
    return (color.r << 16) | (color.g << 8) | color.b;
}

/**
 * @brief Find the window handle for a given LG_WindowHandle
 */
static Window GetWindowHandle(LG_WindowHandle window) {
    if (!window || !window->platform_data) return 0;
    return ((WindowData*)window->platform_data)->window;
}

/**
 * @brief Find the LG_WindowHandle for a given X11 Window
 */
static LG_WindowHandle FindWindowByHandle(Window x_window) {
    // This is a simple implementation that iterates through all windows
    // A more efficient implementation would use a hash table
    extern LG_WindowList g_windows;
    for (size_t i = 0; i < g_windows.count; i++) {
        if (GetWindowHandle(g_windows.windows[i]) == x_window) {
            return g_windows.windows[i];
        }
    }
    return NULL;
}

/**
 * @brief Find the LG_WidgetHandle for a given X11 Window
 */
static LG_WidgetHandle FindWidgetByHandle(Window x_window) {
    // This is a simple implementation that iterates through all windows and widgets
    // A more efficient implementation would use a hash table
    extern LG_WindowList g_windows;
    for (size_t i = 0; i < g_windows.count; i++) {
        LG_WindowHandle window = g_windows.windows[i];
        for (size_t j = 0; j < window->widgets.count; j++) {
            LG_WidgetHandle widget = window->widgets.widgets[j];
            if (widget->platform_data && ((WidgetData*)widget->platform_data)->window == x_window) {
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
        window->event_callback(event, window->user_data);
    }
}

/**
 * @brief Draw a widget
 */
static void DrawWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->platform_data || !widget->window || !widget->window->platform_data) {
        return;
    }

    WidgetData* widget_data = (WidgetData*)widget->platform_data;
    WindowData* window_data = (WindowData*)widget->window->platform_data;
    
    // Set colors
    XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->text_color));
    XSetBackground(g_display, window_data->gc, ColorToX11Color(widget->bg_color));
    
    // Draw widget based on type
    switch (widget->type) {
        case LG_WIDGET_BUTTON:
            // Draw button background
            XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->bg_color));
            XFillRectangle(g_display, widget_data->window, window_data->gc, 
                          0, 0, widget->rect.width, widget->rect.height);
            
            // Draw button border
            XSetForeground(g_display, window_data->gc, ColorToX11Color(LG_COLOR_BLACK));
            XDrawRectangle(g_display, widget_data->window, window_data->gc, 
                          0, 0, widget->rect.width - 1, widget->rect.height - 1);
            
            // Draw button text
            if (widget->text) {
                XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->text_color));
                int text_width = XTextWidth(window_data->font, widget->text, strlen(widget->text));
                int text_x = (widget->rect.width - text_width) / 2;
                int text_y = (widget->rect.height + window_data->font->ascent - window_data->font->descent) / 2;
                XDrawString(g_display, widget_data->window, window_data->gc, 
                           text_x, text_y, widget->text, strlen(widget->text));
            }
            break;
            
        case LG_WIDGET_LABEL:
            // Draw label background (if not transparent)
            if (widget->bg_color.a > 0) {
                XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->bg_color));
                XFillRectangle(g_display, widget_data->window, window_data->gc, 
                              0, 0, widget->rect.width, widget->rect.height);
            }
            
            // Draw label text
            if (widget->text) {
                XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->text_color));
                int text_y = (widget->rect.height + window_data->font->ascent - window_data->font->descent) / 2;
                XDrawString(g_display, widget_data->window, window_data->gc, 
                           5, text_y, widget->text, strlen(widget->text));
            }
            break;
            
        case LG_WIDGET_TEXTFIELD:
            // Draw text field background
            XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->bg_color));
            XFillRectangle(g_display, widget_data->window, window_data->gc, 
                          0, 0, widget->rect.width, widget->rect.height);
            
            // Draw text field border
            XSetForeground(g_display, window_data->gc, ColorToX11Color(LG_COLOR_BLACK));
            XDrawRectangle(g_display, widget_data->window, window_data->gc, 
                          0, 0, widget->rect.width - 1, widget->rect.height - 1);
            
            // Draw text field text
            if (widget->text) {
                XSetForeground(g_display, window_data->gc, ColorToX11Color(widget->text_color));
                int text_y = (widget->rect.height + window_data->font->ascent - window_data->font->descent) / 2;
                XDrawString(g_display, widget_data->window, window_data->gc, 
                           5, text_y, widget->text, strlen(widget->text));
            }
            break;
            
        default:
            break;
    }
    
    XFlush(g_display);
}

/* ========================================================================= */
/*                        Platform API Implementation                        */
/* ========================================================================= */

bool LG_PlatformInitialize(void) {
    // Open display
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        fprintf(stderr, "LightGUI: Failed to open X display\n");
        return false;
    }
    
    // Get default screen
    g_screen = DefaultScreen(g_display);
    
    // Get WM_DELETE_WINDOW atom
    g_wm_delete_window = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    
    // Load default font
    g_default_font = XLoadQueryFont(g_display, "fixed");
    if (!g_default_font) {
        fprintf(stderr, "LightGUI: Failed to load default font\n");
        XCloseDisplay(g_display);
        g_display = NULL;
        return false;
    }
    
    return true;
}

void LG_PlatformTerminate(void) {
    if (g_default_font) {
        XFreeFont(g_display, g_default_font);
        g_default_font = NULL;
    }
    
    if (g_display) {
        XCloseDisplay(g_display);
        g_display = NULL;
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
    
    // Create X window
    XSetWindowAttributes attr;
    attr.background_pixel = WhitePixel(g_display, g_screen);
    attr.border_pixel = BlackPixel(g_display, g_screen);
    attr.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      KeyPressMask | KeyReleaseMask | PointerMotionMask |
                      StructureNotifyMask;
    
    data->window = XCreateWindow(
        g_display,                  // Display
        RootWindow(g_display, g_screen), // Parent
        0, 0,                       // Position
        window->width, window->height, // Size
        1,                          // Border width
        DefaultDepth(g_display, g_screen), // Depth
        InputOutput,                // Class
        DefaultVisual(g_display, g_screen), // Visual
        CWBackPixel | CWBorderPixel | CWEventMask, // Value mask
        &attr                       // Attributes
    );
    
    if (!data->window) {
        fprintf(stderr, "LightGUI: Failed to create X window\n");
        free(data);
        return false;
    }
    
    // Set window title
    XStoreName(g_display, data->window, window->title);
    
    // Set WM_DELETE_WINDOW protocol
    XSetWMProtocols(g_display, data->window, &g_wm_delete_window, 1);
    
    // Create GC
    data->gc = XCreateGC(g_display, data->window, 0, NULL);
    if (!data->gc) {
        fprintf(stderr, "LightGUI: Failed to create GC\n");
        XDestroyWindow(g_display, data->window);
        free(data);
        return false;
    }
    
    // Set font
    data->font = XLoadQueryFont(g_display, "fixed");
    if (!data->font) {
        data->font = g_default_font;
    }
    XSetFont(g_display, data->gc, data->font->fid);
    
    // Create buffer for double buffering
    data->buffer = XCreatePixmap(
        g_display,
        data->window,
        window->width,
        window->height,
        DefaultDepth(g_display, g_screen)
    );
    
    if (!data->buffer) {
        fprintf(stderr, "LightGUI: Failed to create buffer\n");
        XFreeGC(g_display, data->gc);
        XDestroyWindow(g_display, data->window);
        free(data);
        return false;
    }
    
    // Store platform data in window
    window->platform_data = data;
    
    return true;
}

void LG_PlatformDestroyWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    
    // Free resources
    XFreePixmap(g_display, data->buffer);
    
    if (data->font != g_default_font) {
        XFreeFont(g_display, data->font);
    }
    
    XFreeGC(g_display, data->gc);
    XDestroyWindow(g_display, data->window);
    
    free(data);
    window->platform_data = NULL;
}

void LG_PlatformShowWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    XMapWindow(g_display, data->window);
    XFlush(g_display);
}

void LG_PlatformHideWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    XUnmapWindow(g_display, data->window);
    XFlush(g_display);
}

void LG_PlatformSetWindowTitle(LG_WindowHandle window, const char* title) {
    if (!window || !window->platform_data || !title) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    XStoreName(g_display, data->window, title);
    XFlush(g_display);
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
    
    // Create X window for widget
    XSetWindowAttributes attr;
    attr.background_pixel = ColorToX11Color(widget->bg_color);
    attr.border_pixel = BlackPixel(g_display, g_screen);
    attr.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      KeyPressMask | KeyReleaseMask | PointerMotionMask;
    
    data->window = XCreateWindow(
        g_display,                  // Display
        window_data->window,        // Parent
        widget->rect.x, widget->rect.y, // Position
        widget->rect.width, widget->rect.height, // Size
        0,                          // Border width
        DefaultDepth(g_display, g_screen), // Depth
        InputOutput,                // Class
        DefaultVisual(g_display, g_screen), // Visual
        CWBackPixel | CWBorderPixel | CWEventMask, // Value mask
        &attr                       // Attributes
    );
    
    if (!data->window) {
        fprintf(stderr, "LightGUI: Failed to create widget window\n");
        free(data);
        return false;
    }
    
    // Store widget type
    data->type = widget->type;
    
    // Store platform data in widget
    widget->platform_data = data;
    
    // Map widget window
    if (widget->visible) {
        XMapWindow(g_display, data->window);
    }
    
    // Draw widget
    DrawWidget(widget);
    
    XFlush(g_display);
    
    return true;
}

void LG_PlatformDestroyWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->platform_data) return;
    
    WidgetData* data = (WidgetData*)widget->platform_data;
    
    // Destroy widget window
    XDestroyWindow(g_display, data->window);
    
    free(data);
    widget->platform_data = NULL;
}

void LG_PlatformUpdateWidget(LG_WidgetHandle widget) {
    if (!widget || !widget->platform_data) return;
    
    WidgetData* data = (WidgetData*)widget->platform_data;
    
    // Update position and size
    XMoveResizeWindow(g_display, data->window, 
                     widget->rect.x, widget->rect.y, 
                     widget->rect.width, widget->rect.height);
    
    // Update visibility
    if (widget->visible) {
        XMapWindow(g_display, data->window);
    } else {
        XUnmapWindow(g_display, data->window);
    }
    
    // Redraw widget
    DrawWidget(widget);
    
    XFlush(g_display);
}

bool LG_PlatformProcessEvents(void) {
    if (!g_display) return false;
    
    // Process all pending events
    while (XPending(g_display)) {
        XEvent event;
        XNextEvent(g_display, &event);
        
        // Find window
        LG_WindowHandle window = NULL;
        LG_WidgetHandle widget = NULL;
        
        if (event.xany.window) {
            window = FindWindowByHandle(event.xany.window);
            if (!window) {
                widget = FindWidgetByHandle(event.xany.window);
                if (widget) {
                    window = widget->window;
                }
            }
        }
        
        if (!window) continue;
        
        // Process event
        switch (event.type) {
            case Expose:
                if (widget) {
                    DrawWidget(widget);
                } else {
                    LG_PlatformRenderWindow(window);
                }
                break;
                
            case ConfigureNotify:
                if (!widget) {
                    window->width = event.xconfigure.width;
                    window->height = event.xconfigure.height;
                    
                    // Resize buffer
                    WindowData* data = (WindowData*)window->platform_data;
                    XFreePixmap(g_display, data->buffer);
                    data->buffer = XCreatePixmap(
                        g_display,
                        data->window,
                        window->width,
                        window->height,
                        DefaultDepth(g_display, g_screen)
                    );
                    
                    // Dispatch resize event
                    LG_Event lg_event;
                    lg_event.type = LG_EVENT_WINDOW_RESIZE;
                    lg_event.data.window_resize.width = window->width;
                    lg_event.data.window_resize.height = window->height;
                    DispatchEvent(window, &lg_event);
                }
                break;
                
            case ButtonPress:
            case ButtonRelease:
                {
                    LG_Event lg_event;
                    lg_event.type = LG_EVENT_MOUSE_BUTTON;
                    lg_event.data.mouse_button.x = event.xbutton.x;
                    lg_event.data.mouse_button.y = event.xbutton.y;
                    lg_event.data.mouse_button.pressed = (event.type == ButtonPress);
                    
                    switch (event.xbutton.button) {
                        case Button1:
                            lg_event.data.mouse_button.button = LG_MOUSE_BUTTON_LEFT;
                            break;
                        case Button2:
                            lg_event.data.mouse_button.button = LG_MOUSE_BUTTON_MIDDLE;
                            break;
                        case Button3:
                            lg_event.data.mouse_button.button = LG_MOUSE_BUTTON_RIGHT;
                            break;
                        default:
                            continue;  // Ignore other buttons
                    }
                    
                    // If this is a button click on a widget, dispatch a widget clicked event
                    if (widget && event.type == ButtonPress && 
                        lg_event.data.mouse_button.button == LG_MOUSE_BUTTON_LEFT) {
                        LG_Event widget_event;
                        widget_event.type = LG_EVENT_WIDGET_CLICKED;
                        widget_event.data.widget_clicked.widget = widget;
                        widget_event.data.widget_clicked.x = event.xbutton.x;
                        widget_event.data.widget_clicked.y = event.xbutton.y;
                        DispatchEvent(window, &widget_event);
                    }
                    
                    DispatchEvent(window, &lg_event);
                }
                break;
                
            case MotionNotify:
                {
                    LG_Event lg_event;
                    lg_event.type = LG_EVENT_MOUSE_MOVE;
                    lg_event.data.mouse_move.x = event.xmotion.x;
                    lg_event.data.mouse_move.y = event.xmotion.y;
                    
                    // Calculate delta (this is simplified)
                    static int last_x = 0, last_y = 0;
                    lg_event.data.mouse_move.delta_x = lg_event.data.mouse_move.x - last_x;
                    lg_event.data.mouse_move.delta_y = lg_event.data.mouse_move.y - last_y;
                    last_x = lg_event.data.mouse_move.x;
                    last_y = lg_event.data.mouse_move.y;
                    
                    DispatchEvent(window, &lg_event);
                }
                break;
                
            case KeyPress:
            case KeyRelease:
                {
                    KeySym keysym = XLookupKeysym(&event.xkey, 0);
                    
                    LG_Event lg_event;
                    lg_event.type = LG_EVENT_KEY;
                    lg_event.data.key.key_code = (int)keysym;
                    lg_event.data.key.pressed = (event.type == KeyPress);
                    lg_event.data.key.ctrl = (event.xkey.state & ControlMask) != 0;
                    lg_event.data.key.shift = (event.xkey.state & ShiftMask) != 0;
                    lg_event.data.key.alt = (event.xkey.state & Mod1Mask) != 0;
                    
                    DispatchEvent(window, &lg_event);
                }
                break;
                
            case ClientMessage:
                if (event.xclient.data.l[0] == g_wm_delete_window) {
                    LG_Event lg_event;
                    lg_event.type = LG_EVENT_WINDOW_CLOSE;
                    DispatchEvent(window, &lg_event);
                }
                break;
        }
    }
    
    return true;
}

void LG_PlatformRenderWindow(LG_WindowHandle window) {
    if (!window || !window->platform_data) return;
    
    WindowData* data = (WindowData*)window->platform_data;
    
    // Clear buffer
    XSetForeground(g_display, data->gc, WhitePixel(g_display, g_screen));
    XFillRectangle(g_display, data->buffer, data->gc, 0, 0, window->width, window->height);
    
    // Copy buffer to window
    XCopyArea(g_display, data->buffer, data->window, data->gc, 
             0, 0, window->width, window->height, 0, 0);
    
    XFlush(g_display);
}

#endif /* __linux__ */ 