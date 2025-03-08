/**
 * @file simple_form.c
 * @brief Enhanced form example using the LightGUI framework
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Widget handles
LG_WidgetHandle title_label = NULL;
LG_WidgetHandle name_label = NULL;
LG_WidgetHandle name_field = NULL;
LG_WidgetHandle email_label = NULL;
LG_WidgetHandle email_field = NULL;
LG_WidgetHandle age_label = NULL;
LG_WidgetHandle age_field = NULL;
LG_WidgetHandle submit_button = NULL;
LG_WidgetHandle clear_button = NULL;
LG_WidgetHandle status_label = NULL;
LG_WidgetHandle timestamp_label = NULL;

// Application state
bool form_submitted = false;
time_t last_update_time = 0;

/**
 * @brief Update the timestamp label with current time
 */
void update_timestamp() {
    time_t current_time = time(NULL);
    
    // Only update once per second to avoid excessive updates
    if (current_time != last_update_time) {
        char time_str[64];
        struct tm* time_info = localtime(&current_time);
        strftime(time_str, sizeof(time_str), "Current time: %H:%M:%S", time_info);
        
        LG_SetWidgetText(timestamp_label, time_str);
        last_update_time = current_time;
    }
}

/**
 * @brief Clear all form fields
 */
void clear_form() {
    LG_SetWidgetText(name_field, "");
    LG_SetWidgetText(email_field, "");
    LG_SetWidgetText(age_field, "");
    LG_SetWidgetText(status_label, "Form cleared");
    
    // Reset form state
    form_submitted = false;
}

/**
 * @brief Validate email format (basic check)
 */
bool is_valid_email(const char* email) {
    return email && strchr(email, '@') && strchr(email, '.');
}

/**
 * @brief Validate age (must be a number between 18-120)
 */
bool is_valid_age(const char* age_str) {
    if (!age_str || !*age_str) return false;
    
    // Check if all characters are digits
    for (const char* p = age_str; *p; p++) {
        if (*p < '0' || *p > '9') return false;
    }
    
    // Convert to integer and check range
    int age = atoi(age_str);
    return age >= 18 && age <= 120;
}

/**
 * @brief Submit the form
 */
void submit_form() {
    char name[256] = {0};
    char email[256] = {0};
    char age_str[16] = {0};
    
    LG_GetWidgetText(name_field, name, sizeof(name));
    LG_GetWidgetText(email_field, email, sizeof(email));
    LG_GetWidgetText(age_field, age_str, sizeof(age_str));
    
    printf("Form submitted: name=%s, email=%s, age=%s\n", name, email, age_str);
    
    // Validate input
    if (strlen(name) < 2) {
        LG_SetWidgetText(status_label, "Error: Name must be at least 2 characters");
        return;
    }
    
    if (!is_valid_email(email)) {
        LG_SetWidgetText(status_label, "Error: Invalid email address");
        return;
    }
    
    if (!is_valid_age(age_str)) {
        LG_SetWidgetText(status_label, "Error: Age must be between 18 and 120");
        return;
    }
    
    // All validations passed
    char success_msg[512];
    snprintf(success_msg, sizeof(success_msg), 
             "Success! Thank you, %s. Your information has been submitted.", name);
    LG_SetWidgetText(status_label, success_msg);
    
    // Update form state
    form_submitted = true;
    
    // Change button colors to indicate success
    LG_Color success_color = LG_CreateColor(0, 180, 0, 255);
    LG_SetWidgetBackgroundColor(submit_button, success_color);
    LG_SetWidgetTextColor(submit_button, LG_COLOR_WHITE);
}

/**
 * @brief Event callback function
 */
void event_callback(LG_Event* event, void* user_data) {
    // Only print specific event types to reduce console spam
    if (event->type == LG_EVENT_WINDOW_CLOSE || 
        event->type == LG_EVENT_WIDGET_CLICKED) {
        printf("Event received: type=%d\n", event->type);
    }
    
    switch (event->type) {
        case LG_EVENT_WINDOW_CLOSE:
            printf("Window close event received\n");
            break;
            
        case LG_EVENT_WIDGET_CLICKED:
            {
                LG_WidgetHandle widget = event->data.widget_clicked.widget;
                
                if (widget == submit_button) {
                    submit_form();
                } else if (widget == clear_button) {
                    clear_form();
                    
                    // Reset submit button colors
                    LG_SetWidgetBackgroundColor(submit_button, LG_COLOR_WHITE);
                    LG_SetWidgetTextColor(submit_button, LG_COLOR_BLACK);
                }
            }
            break;
            
        case LG_EVENT_KEY:
            // Handle Enter key to submit form
            if (event->data.key.key_code == 13 && event->data.key.pressed) {
                submit_form();
            }
            break;
    }
    
    // Update timestamp on every event
    update_timestamp();
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    LG_WindowHandle window = LG_CreateWindow("Enhanced Form Example", 500, 400, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create title
    title_label = LG_CreateLabel(window, "User Registration Form", 20, 20, 460, 30);
    LG_Color title_color = LG_CreateColor(0, 0, 128, 255); // Dark blue
    LG_SetWidgetTextColor(title_label, title_color);
    
    // Create form fields
    int y_pos = 70;
    int label_width = 120;
    int field_width = 300;
    int field_height = 25;
    int spacing = 40;
    
    // Name field
    name_label = LG_CreateLabel(window, "Name:", 20, y_pos, label_width, field_height);
    name_field = LG_CreateTextField(window, "", 150, y_pos, field_width, field_height);
    y_pos += spacing;
    
    // Email field
    email_label = LG_CreateLabel(window, "Email:", 20, y_pos, label_width, field_height);
    email_field = LG_CreateTextField(window, "", 150, y_pos, field_width, field_height);
    y_pos += spacing;
    
    // Age field
    age_label = LG_CreateLabel(window, "Age:", 20, y_pos, label_width, field_height);
    age_field = LG_CreateTextField(window, "", 150, y_pos, field_width, field_height);
    y_pos += spacing + 10;
    
    // Buttons
    submit_button = LG_CreateButton(window, "Submit", 150, y_pos, 120, 35);
    clear_button = LG_CreateButton(window, "Clear", 290, y_pos, 120, 35);
    y_pos += spacing + 20;
    
    // Status label
    status_label = LG_CreateLabel(window, "Please fill in all fields and submit", 20, y_pos, 460, field_height);
    LG_Color status_color = LG_CreateColor(128, 0, 0, 255); // Dark red
    LG_SetWidgetTextColor(status_label, status_color);
    y_pos += spacing;
    
    // Timestamp label
    timestamp_label = LG_CreateLabel(window, "", 20, y_pos, 460, field_height);
    LG_Color timestamp_color = LG_CreateColor(100, 100, 100, 255); // Gray
    LG_SetWidgetTextColor(timestamp_label, timestamp_color);
    
    // Initialize timestamp
    update_timestamp();
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 