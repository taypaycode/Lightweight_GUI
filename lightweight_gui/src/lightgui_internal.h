/**
 * @file lightgui_internal.h
 * @brief Internal header file for the LightGUI framework
 * 
 * This header defines internal structures and functions that are not part
 * of the public API.
 */

#ifndef LIGHTGUI_INTERNAL_H
#define LIGHTGUI_INTERNAL_H

#include "../include/lightgui.h"
#include <stddef.h>

/* ========================================================================= */
/*                        Internal Helper Functions                          */
/* ========================================================================= */

/* Global window list - declared here, defined in lightgui.c */
extern LG_WindowList g_windows;

/**
 * @brief Add a widget to a window
 * 
 * @param window The window
 * @param widget The widget to add
 */
void AddWidgetToWindow(LG_WindowHandle window, LG_WidgetHandle widget);

#endif /* LIGHTGUI_INTERNAL_H */