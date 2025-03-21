/**
 * @file simple_form.c
 * @brief Enhanced form example using the LightGUI framework
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#define PATH_SEPARATOR "\\"
#else
#include <sys/stat.h>
#define PATH_SEPARATOR "/"
#endif

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 400
#define DATA_DIR "user_data"
#define USER_FILE "user_data/users.csv"
#define MAX_USERS 100
#define MAX_LINE 1024

// Widget handles
LG_WindowHandle window;
LG_WidgetHandle name_label;
LG_WidgetHandle name_field;
LG_WidgetHandle email_label;
LG_WidgetHandle email_field;
LG_WidgetHandle submit_button;
LG_WidgetHandle clear_button;
LG_WidgetHandle status_label;
LG_WidgetHandle user_list_label;
LG_WidgetHandle user_list;
LG_WidgetHandle delete_button;
LG_WidgetHandle edit_button;

// User data structure
typedef struct {
    char name[256];
    char email[256];
    char created_at[64];
} User;

User users[MAX_USERS];
int user_count = 0;
int selected_user = -1;
bool edit_mode = false;

/**
 * @brief Create data directory if it doesn't exist
 */
bool ensure_data_directory() {
    #ifdef _WIN32
    if (_mkdir(DATA_DIR) != 0) {
        // Check if directory already exists
        if (GetFileAttributes(DATA_DIR) == INVALID_FILE_ATTRIBUTES) {
            printf("Error: Failed to create data directory.\n");
            return false;
        }
    }
    #else
    if (mkdir(DATA_DIR, 0755) != 0) {
        // Check if directory already exists
        struct stat st;
        if (stat(DATA_DIR, &st) != 0 || !S_ISDIR(st.st_mode)) {
            printf("Error: Failed to create data directory.\n");
            return false;
        }
    }
    #endif
    return true;
}

/**
 * @brief Save users to CSV file
 */
bool save_users() {
    if (!ensure_data_directory()) {
        return false;
    }
    
    FILE* file = fopen(USER_FILE, "w");
    if (!file) {
        printf("Error: Failed to open user file for writing.\n");
        return false;
    }
    
    // Write header
    fprintf(file, "Name,Email,Created At\n");
    
    // Write user data
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "\"%s\",\"%s\",\"%s\"\n", 
                users[i].name, users[i].email, users[i].created_at);
    }
    
    fclose(file);
    return true;
}

/**
 * @brief Load users from CSV file
 */
bool load_users() {
    FILE* file = fopen(USER_FILE, "r");
    if (!file) {
        // It's okay if the file doesn't exist yet
        return true;
    }
    
    char line[MAX_LINE];
    user_count = 0;
    
    // Skip header
    if (fgets(line, MAX_LINE, file) == NULL) {
        fclose(file);
        return true;
    }
    
    // Read user data
    while (fgets(line, MAX_LINE, file) != NULL && user_count < MAX_USERS) {
        char* name_start = strchr(line, '\"');
        if (!name_start) continue;
        name_start++; // Skip the opening quote
        
        char* name_end = strchr(name_start, '\"');
        if (!name_end) continue;
        *name_end = '\0';
        strcpy(users[user_count].name, name_start);
        
        char* email_start = strchr(name_end + 1, '\"');
        if (!email_start) continue;
        email_start++; // Skip the opening quote
        
        char* email_end = strchr(email_start, '\"');
        if (!email_end) continue;
        *email_end = '\0';
        strcpy(users[user_count].email, email_start);
        
        char* created_start = strchr(email_end + 1, '\"');
        if (!created_start) continue;
        created_start++; // Skip the opening quote
        
        char* created_end = strchr(created_start, '\"');
        if (!created_end) continue;
        *created_end = '\0';
        strcpy(users[user_count].created_at, created_start);
        
        user_count++;
    }
    
    fclose(file);
    return true;
}

/**
 * @brief Update the user list widget
 */
void update_user_list() {
    char list_text[MAX_USERS * 100];
    list_text[0] = '\0';
    
    for (int i = 0; i < user_count; i++) {
        char user_text[100];
        snprintf(user_text, sizeof(user_text), "%d. %s (%s)\n", 
                i + 1, users[i].name, users[i].email);
        strcat(list_text, user_text);
    }
    
    if (user_count == 0) {
        strcat(list_text, "No users registered yet.");
    }
    
    LG_SetWidgetText(user_list, list_text);
}

/**
 * @brief Add a new user
 */
void add_user(const char* name, const char* email) {
    if (user_count >= MAX_USERS) {
        LG_SetWidgetText(status_label, "Error: Maximum number of users reached.");
        return;
    }
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Create new user
    strcpy(users[user_count].name, name);
    strcpy(users[user_count].email, email);
    strcpy(users[user_count].created_at, timestamp);
    user_count++;
    
    // Save to file
    if (save_users()) {
        char status[256];
        snprintf(status, sizeof(status), "User '%s' added successfully!", name);
        LG_SetWidgetText(status_label, status);
    } else {
        LG_SetWidgetText(status_label, "Error: Failed to save user data.");
    }
    
    // Update list
    update_user_list();
}

/**
 * @brief Update an existing user
 */
void update_user(int index, const char* name, const char* email) {
    if (index < 0 || index >= user_count) {
        LG_SetWidgetText(status_label, "Error: Invalid user index.");
        return;
    }
    
    // Update user data (keep original timestamp)
    strcpy(users[index].name, name);
    strcpy(users[index].email, email);
    
    // Save to file
    if (save_users()) {
        char status[256];
        snprintf(status, sizeof(status), "User '%s' updated successfully!", name);
        LG_SetWidgetText(status_label, status);
    } else {
        LG_SetWidgetText(status_label, "Error: Failed to save user data.");
    }
    
    // Update list
    update_user_list();
}

/**
 * @brief Delete a user
 */
void delete_user(int index) {
    if (index < 0 || index >= user_count) {
        LG_SetWidgetText(status_label, "Error: Invalid user index.");
        return;
    }
    
    char deleted_name[256];
    strcpy(deleted_name, users[index].name);
    
    // Remove user by shifting all users after it
    for (int i = index; i < user_count - 1; i++) {
        strcpy(users[i].name, users[i + 1].name);
        strcpy(users[i].email, users[i + 1].email);
        strcpy(users[i].created_at, users[i + 1].created_at);
    }
    user_count--;
    
    // Save to file
    if (save_users()) {
        char status[256];
        snprintf(status, sizeof(status), "User '%s' deleted successfully!", deleted_name);
        LG_SetWidgetText(status_label, status);
    } else {
        LG_SetWidgetText(status_label, "Error: Failed to save user data.");
    }
    
    // Update list
    update_user_list();
    
    // Clear form and reset selection
    LG_SetWidgetText(name_field, "");
    LG_SetWidgetText(email_field, "");
    selected_user = -1;
    edit_mode = false;
    LG_SetWidgetText(submit_button, "Submit");
}

/**
 * @brief Clear the form
 */
void clear_form() {
    LG_SetWidgetText(name_field, "");
    LG_SetWidgetText(email_field, "");
    LG_SetWidgetText(status_label, "Form cleared.");
    selected_user = -1;
    edit_mode = false;
    LG_SetWidgetText(submit_button, "Submit");
}

/**
 * @brief Check if a string contains a valid email address
 */
bool is_valid_email(const char* email) {
    // Very basic validation: check for @ and . characters
    const char* at = strchr(email, '@');
    if (!at) return false;
    
    const char* dot = strchr(at, '.');
    if (!dot) return false;
    
    // Check email is not too short and that dot comes after @
    return (at > email) && (dot > at + 1) && (dot < email + strlen(email) - 1);
}

/**
 * @brief Handle list selection
 */
void handle_list_selection(int x, int y) {
    // Convert y position to user index
    // This is a simplistic approach - a real implementation would need
    // to handle scrolling and proper line height calculations
    int line_height = 20; // Approximate line height
    int first_line_y = 0; // Top of the first line
    
    int rel_y = y - first_line_y;
    if (rel_y < 0) return;
    
    int line_index = rel_y / line_height;
    if (line_index < user_count) {
        selected_user = line_index;
        
        // Fill form with selected user data
        LG_SetWidgetText(name_field, users[selected_user].name);
        LG_SetWidgetText(email_field, users[selected_user].email);
        
        // Change to edit mode
        edit_mode = true;
        LG_SetWidgetText(submit_button, "Update");
        
        char status[256];
        snprintf(status, sizeof(status), "Editing user: %s", users[selected_user].name);
        LG_SetWidgetText(status_label, status);
    }
}

/**
 * @brief Event callback function
 */
void event_callback(LG_Event* event, void* user_data) {
    switch (event->type) {
        case LG_EVENT_WINDOW_CLOSE:
            printf("Window close event received.\n");
            break;
            
        case LG_EVENT_WIDGET_CLICKED:
            {
                LG_WidgetHandle widget = event->data.widget_clicked.widget;
                
                if (widget == submit_button) {
                    // Get form values
                    char name[256];
                    char email[256];
                    LG_GetWidgetText(name_field, name, sizeof(name));
                    LG_GetWidgetText(email_field, email, sizeof(email));
                    
                    // Validate input
                    if (strlen(name) < 3) {
                        LG_SetWidgetText(status_label, "Error: Name must be at least 3 characters");
                        return;
                    }
                    
                    if (!is_valid_email(email)) {
                        LG_SetWidgetText(status_label, "Error: Please enter a valid email address");
                        return;
                    }
                    
                    // Add or update user
                    if (edit_mode && selected_user >= 0) {
                        update_user(selected_user, name, email);
                        edit_mode = false;
                        LG_SetWidgetText(submit_button, "Submit");
                    } else {
                        add_user(name, email);
                    }
                    
                    // Clear form
                    LG_SetWidgetText(name_field, "");
                    LG_SetWidgetText(email_field, "");
                    selected_user = -1;
                }
                else if (widget == clear_button) {
                    clear_form();
                }
                else if (widget == delete_button) {
                    if (selected_user >= 0) {
                        delete_user(selected_user);
                    } else {
                        LG_SetWidgetText(status_label, "Error: No user selected to delete");
                    }
                }
                else if (widget == edit_button) {
                    if (selected_user >= 0) {
                        // Form is already filled, just ensure we're in edit mode
                        edit_mode = true;
                        LG_SetWidgetText(submit_button, "Update");
                        
                        char status[256];
                        snprintf(status, sizeof(status), "Editing user: %s", users[selected_user].name);
                        LG_SetWidgetText(status_label, status);
                    } else {
                        LG_SetWidgetText(status_label, "Error: No user selected to edit");
                    }
                }
                else if (widget == user_list) {
                    handle_list_selection(event->data.widget_clicked.x, event->data.widget_clicked.y);
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
    window = LG_CreateWindow("User Registration Form", WINDOW_WIDTH, WINDOW_HEIGHT, false);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create widgets - Form section
    int form_width = 230;
    int left_margin = 20;
    int top_margin = 20;
    int field_height = 25;
    int spacing = 10;
    
    // Form title
    LG_WidgetHandle form_title = LG_CreateLabel(window, "User Registration Form", 
                                               left_margin, top_margin, form_width, 30);
    LG_SetWidgetTextColor(form_title, LG_CreateColor(0, 0, 128, 255));
    
    // Name
    name_label = LG_CreateLabel(window, "Name:", 
                              left_margin, top_margin + 40, 80, field_height);
    name_field = LG_CreateTextField(window, "", 
                                  left_margin + 90, top_margin + 40, form_width - 90, field_height);
    
    // Email
    email_label = LG_CreateLabel(window, "Email:", 
                               left_margin, top_margin + 40 + (field_height + spacing), 80, field_height);
    email_field = LG_CreateTextField(window, "", 
                                   left_margin + 90, top_margin + 40 + (field_height + spacing), 
                                   form_width - 90, field_height);
    
    // Submit and Clear buttons
    submit_button = LG_CreateButton(window, "Submit", 
                                  left_margin, top_margin + 40 + 2 * (field_height + spacing), 
                                  100, 30);
    clear_button = LG_CreateButton(window, "Clear", 
                                 left_margin + 110, top_margin + 40 + 2 * (field_height + spacing), 
                                 100, 30);
    
    // Status label
    status_label = LG_CreateLabel(window, "Ready to submit form", 
                                left_margin, top_margin + 40 + 3 * (field_height + spacing), 
                                form_width, field_height);
    
    // Create widgets - User list section
    int list_left = left_margin + form_width + 20;
    int list_width = WINDOW_WIDTH - list_left - 20;
    
    user_list_label = LG_CreateLabel(window, "Registered Users:", 
                                   list_left, top_margin, list_width, 30);
    
    user_list = LG_CreateLabel(window, "Loading users...", 
                             list_left, top_margin + 40, list_width, 
                             WINDOW_HEIGHT - top_margin - 100);
    LG_SetWidgetBackgroundColor(user_list, LG_CreateColor(240, 240, 240, 255));
    
    // Edit and Delete buttons
    edit_button = LG_CreateButton(window, "Edit Selected", 
                                list_left, WINDOW_HEIGHT - 50, 
                                100, 30);
    delete_button = LG_CreateButton(window, "Delete Selected", 
                                  list_left + 110, WINDOW_HEIGHT - 50, 
                                  100, 30);
    
    // Load user data
    load_users();
    update_user_list();
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 