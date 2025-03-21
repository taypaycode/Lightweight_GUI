/**
 * @file calculator.c
 * @brief Calculator application using the LightGUI framework
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 400
#define MAX_DISPLAY_LENGTH 24

// Operation enum
typedef enum {
    OP_NONE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_PERCENT,
    OP_SQRT
} Operation;

// Application state
LG_WindowHandle window = NULL;
LG_WidgetHandle display = NULL;
LG_WidgetHandle digit_buttons[10];
LG_WidgetHandle op_buttons[8];
LG_WidgetHandle decimal_button = NULL;
LG_WidgetHandle clear_button = NULL;
LG_WidgetHandle equals_button = NULL;
LG_WidgetHandle backspace_button = NULL;

// Calculator state
char display_value[MAX_DISPLAY_LENGTH + 1] = "0";
double current_value = 0.0;
double stored_value = 0.0;
Operation current_operation = OP_NONE;
bool new_input = true;
bool has_decimal = false;

/**
 * @brief Update the display widget
 */
void update_display() {
    LG_SetWidgetText(display, display_value);
}

/**
 * @brief Clear the calculator
 */
void clear_calc() {
    strcpy(display_value, "0");
    current_value = 0.0;
    stored_value = 0.0;
    current_operation = OP_NONE;
    new_input = true;
    has_decimal = false;
    update_display();
}

/**
 * @brief Add a digit to the display
 */
void add_digit(int digit) {
    if (new_input) {
        strcpy(display_value, "");
        new_input = false;
        has_decimal = false;
    }
    
    // Check if we've reached the maximum display length
    if (strlen(display_value) >= MAX_DISPLAY_LENGTH) {
        return;
    }
    
    // Append the digit
    char digit_str[2] = {digit + '0', '\0'};
    strcat(display_value, digit_str);
    
    // Update the display
    update_display();
}

/**
 * @brief Add a decimal point
 */
void add_decimal() {
    if (has_decimal) {
        return;
    }
    
    if (new_input) {
        strcpy(display_value, "0");
        new_input = false;
    }
    
    // Check if we've reached the maximum display length
    if (strlen(display_value) >= MAX_DISPLAY_LENGTH) {
        return;
    }
    
    // Append the decimal point
    strcat(display_value, ".");
    has_decimal = true;
    
    // Update the display
    update_display();
}

/**
 * @brief Remove the last character from the display
 */
void backspace() {
    size_t len = strlen(display_value);
    
    if (len <= 1) {
        // If only one character, set to 0
        strcpy(display_value, "0");
        new_input = true;
    } else {
        // Remove the last character
        display_value[len - 1] = '\0';
        
        // Check if we removed the decimal point
        if (display_value[len - 2] == '.') {
            has_decimal = false;
        }
    }
    
    // Update the display
    update_display();
}

/**
 * @brief Perform the current operation
 */
void perform_operation() {
    double display_num = atof(display_value);
    
    switch (current_operation) {
        case OP_ADD:
            current_value = stored_value + display_num;
            break;
        case OP_SUBTRACT:
            current_value = stored_value - display_num;
            break;
        case OP_MULTIPLY:
            current_value = stored_value * display_num;
            break;
        case OP_DIVIDE:
            if (display_num != 0) {
                current_value = stored_value / display_num;
            } else {
                strcpy(display_value, "Error: Divide by zero");
                update_display();
                new_input = true;
                return;
            }
            break;
        case OP_PERCENT:
            current_value = stored_value * (display_num / 100.0);
            break;
        case OP_SQRT:
            if (display_num >= 0) {
                current_value = sqrt(display_num);
            } else {
                strcpy(display_value, "Error: Invalid input");
                update_display();
                new_input = true;
                return;
            }
            break;
        case OP_NONE:
            current_value = display_num;
            break;
    }
    
    // Convert the result to string
    snprintf(display_value, MAX_DISPLAY_LENGTH, "%.15g", current_value);
    
    // Update the display
    update_display();
    new_input = true;
}

/**
 * @brief Set the current operation
 */
void set_operation(Operation op) {
    // If there's a pending operation, perform it first
    if (current_operation != OP_NONE && !new_input) {
        perform_operation();
    }
    
    // Store the current value
    stored_value = atof(display_value);
    
    // Set the new operation
    current_operation = op;
    new_input = true;
    
    // For square root, perform immediately
    if (op == OP_SQRT) {
        perform_operation();
        current_operation = OP_NONE;
    }
}

/**
 * @brief Create a button with a label
 */
LG_WidgetHandle create_button(const char* label, int x, int y, int width, int height) {
    LG_WidgetHandle button = LG_CreateButton(window, label, x, y, width, height);
    return button;
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
                
                // Check digit buttons
                for (int i = 0; i < 10; i++) {
                    if (widget == digit_buttons[i]) {
                        add_digit(i);
                        return;
                    }
                }
                
                // Check operator buttons
                if (widget == op_buttons[0]) { // +
                    set_operation(OP_ADD);
                } else if (widget == op_buttons[1]) { // -
                    set_operation(OP_SUBTRACT);
                } else if (widget == op_buttons[2]) { // *
                    set_operation(OP_MULTIPLY);
                } else if (widget == op_buttons[3]) { // /
                    set_operation(OP_DIVIDE);
                } else if (widget == op_buttons[4]) { // %
                    set_operation(OP_PERCENT);
                } else if (widget == op_buttons[5]) { // √
                    set_operation(OP_SQRT);
                } else if (widget == equals_button) {
                    if (current_operation != OP_NONE) {
                        perform_operation();
                        current_operation = OP_NONE;
                    }
                } else if (widget == decimal_button) {
                    add_decimal();
                } else if (widget == clear_button) {
                    clear_calc();
                } else if (widget == backspace_button) {
                    backspace();
                }
            }
            break;
            
        case LG_EVENT_KEY:
            if (event->data.key.pressed) {
                int key = event->data.key.key_code;
                
                // Handle digit keys (0-9)
                if (key >= '0' && key <= '9') {
                    add_digit(key - '0');
                } 
                // Handle operators
                else if (key == '+') {
                    set_operation(OP_ADD);
                } else if (key == '-') {
                    set_operation(OP_SUBTRACT);
                } else if (key == '*') {
                    set_operation(OP_MULTIPLY);
                } else if (key == '/') {
                    set_operation(OP_DIVIDE);
                } else if (key == '%') {
                    set_operation(OP_PERCENT);
                } else if (key == '=') {
                    if (current_operation != OP_NONE) {
                        perform_operation();
                        current_operation = OP_NONE;
                    }
                } else if (key == '.') {
                    add_decimal();
                } else if (key == 27) { // ESC
                    clear_calc();
                } else if (key == 8) { // Backspace
                    backspace();
                } else if (key == 13) { // Enter (same as =)
                    if (current_operation != OP_NONE) {
                        perform_operation();
                        current_operation = OP_NONE;
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
    window = LG_CreateWindow("Calculator", WINDOW_WIDTH, WINDOW_HEIGHT, false);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create display
    display = LG_CreateLabel(window, "0", 10, 10, WINDOW_WIDTH - 20, 50);
    LG_SetWidgetBackgroundColor(display, LG_CreateColor(240, 240, 240, 255));
    
    // Create clear and backspace buttons
    clear_button = create_button("C", 10, 70, 90, 40);
    backspace_button = create_button("⌫", 110, 70, 90, 40);
    op_buttons[5] = create_button("√", 210, 70, 80, 40); // Square root
    
    // Create digit buttons (grid layout)
    int button_width = 60;
    int button_height = 50;
    int start_x = 10;
    int start_y = 120;
    int padding = 10;
    
    // Create buttons 7, 8, 9, /
    digit_buttons[7] = create_button("7", start_x, start_y, button_width, button_height);
    digit_buttons[8] = create_button("8", start_x + button_width + padding, start_y, button_width, button_height);
    digit_buttons[9] = create_button("9", start_x + 2 * (button_width + padding), start_y, button_width, button_height);
    op_buttons[3] = create_button("/", start_x + 3 * (button_width + padding), start_y, button_width, button_height);
    
    // Create buttons 4, 5, 6, *
    start_y += button_height + padding;
    digit_buttons[4] = create_button("4", start_x, start_y, button_width, button_height);
    digit_buttons[5] = create_button("5", start_x + button_width + padding, start_y, button_width, button_height);
    digit_buttons[6] = create_button("6", start_x + 2 * (button_width + padding), start_y, button_width, button_height);
    op_buttons[2] = create_button("*", start_x + 3 * (button_width + padding), start_y, button_width, button_height);
    
    // Create buttons 1, 2, 3, -
    start_y += button_height + padding;
    digit_buttons[1] = create_button("1", start_x, start_y, button_width, button_height);
    digit_buttons[2] = create_button("2", start_x + button_width + padding, start_y, button_width, button_height);
    digit_buttons[3] = create_button("3", start_x + 2 * (button_width + padding), start_y, button_width, button_height);
    op_buttons[1] = create_button("-", start_x + 3 * (button_width + padding), start_y, button_width, button_height);
    
    // Create buttons 0, ., =, +
    start_y += button_height + padding;
    digit_buttons[0] = create_button("0", start_x, start_y, button_width, button_height);
    decimal_button = create_button(".", start_x + button_width + padding, start_y, button_width, button_height);
    equals_button = create_button("=", start_x + 2 * (button_width + padding), start_y, button_width, button_height);
    op_buttons[0] = create_button("+", start_x + 3 * (button_width + padding), start_y, button_width, button_height);
    
    // Create percent button
    start_y += button_height + padding;
    op_buttons[4] = create_button("%", start_x, start_y, button_width, button_height);
    
    // Style special buttons
    LG_Color accent_color = LG_CreateColor(0, 120, 215, 255); // Blue
    LG_SetWidgetBackgroundColor(equals_button, accent_color);
    LG_SetWidgetTextColor(equals_button, LG_CreateColor(255, 255, 255, 255));
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 