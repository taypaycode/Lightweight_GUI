/**
 * @file todo_list.c
 * @brief Todo list application using the LightGUI framework
 */

#include "../include/lightgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_TODOS 20
#define TODO_HEIGHT 30
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 600
#define MAX_TASKS 100
#define MAX_TASK_TEXT 256

// Todo item structure
typedef struct {
    char text[256];
    bool completed;
    LG_WidgetHandle checkbox;
    LG_WidgetHandle label;
    LG_WidgetHandle delete_button;
} TodoItem;

// Application state
LG_WindowHandle window = NULL;
LG_WidgetHandle title_label = NULL;
LG_WidgetHandle new_todo_field = NULL;
LG_WidgetHandle add_button = NULL;
LG_WidgetHandle clear_completed_button = NULL;
LG_WidgetHandle status_label = NULL;

TodoItem todos[MAX_TODOS];
int todo_count = 0;
int y_offset = 120; // Starting Y position for todo items

// Colors
LG_Color title_color;
LG_Color completed_color;
LG_Color normal_color;
LG_Color delete_color;

// Task list widgets
typedef struct {
    LG_WidgetHandle checkbox;
    LG_WidgetHandle text_label;
    LG_WidgetHandle delete_button;
    char text[MAX_TASK_TEXT];
    bool completed;
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;
int start_y = 100;
int task_height = 30;
int task_spacing = 10;

/**
 * @brief Update the status label with current count
 */
void update_status() {
    char status[64];
    int completed = 0;
    
    for (int i = 0; i < todo_count; i++) {
        if (todos[i].completed) {
            completed++;
        }
    }
    
    snprintf(status, sizeof(status), "%d items, %d completed", todo_count, completed);
    LG_SetWidgetText(status_label, status);
}

/**
 * @brief Rearrange todos after deletion
 */
void rearrange_todos() {
    // Reset the Y position
    y_offset = 120;
    
    // Reposition all todo items
    for (int i = 0; i < todo_count; i++) {
        LG_SetWidgetPosition(todos[i].checkbox, 20, y_offset);
        LG_SetWidgetPosition(todos[i].label, 50, y_offset);
        LG_SetWidgetPosition(todos[i].delete_button, 450, y_offset);
        y_offset += TODO_HEIGHT;
    }
}

/**
 * @brief Delete a todo item
 */
void delete_todo(int index) {
    if (index < 0 || index >= todo_count) {
        return;
    }
    
    // Destroy widgets
    LG_DestroyWidget(todos[index].checkbox);
    LG_DestroyWidget(todos[index].label);
    LG_DestroyWidget(todos[index].delete_button);
    
    // Shift items down
    for (int i = index; i < todo_count - 1; i++) {
        todos[i] = todos[i + 1];
    }
    
    todo_count--;
    rearrange_todos();
    update_status();
}

/**
 * @brief Toggle completion status of a todo
 */
void toggle_todo(int index) {
    if (index < 0 || index >= todo_count) {
        return;
    }
    
    todos[index].completed = !todos[index].completed;
    
    if (todos[index].completed) {
        LG_SetWidgetTextColor(todos[index].label, completed_color);
    } else {
        LG_SetWidgetTextColor(todos[index].label, normal_color);
    }
    
    update_status();
}

/**
 * @brief Add a new todo item
 */
void add_todo() {
    char todo_text[256];
    LG_GetWidgetText(new_todo_field, todo_text, sizeof(todo_text));
    
    // Don't add empty todos
    if (strlen(todo_text) == 0) {
        return;
    }
    
    // Check if we've reached the maximum
    if (todo_count >= MAX_TODOS) {
        LG_SetWidgetText(status_label, "Maximum number of todos reached!");
        return;
    }
    
    // Create checkbox (simulated with a button)
    char checkbox_text[8] = "[ ]";
    todos[todo_count].checkbox = LG_CreateButton(window, checkbox_text, 20, y_offset, 30, TODO_HEIGHT);
    
    // Create label
    todos[todo_count].label = LG_CreateLabel(window, todo_text, 50, y_offset, 380, TODO_HEIGHT);
    LG_SetWidgetTextColor(todos[todo_count].label, normal_color);
    
    // Create delete button
    todos[todo_count].delete_button = LG_CreateButton(window, "X", 450, y_offset, 30, TODO_HEIGHT);
    LG_SetWidgetTextColor(todos[todo_count].delete_button, delete_color);
    
    // Store todo data
    strncpy(todos[todo_count].text, todo_text, sizeof(todos[todo_count].text) - 1);
    todos[todo_count].completed = false;
    
    // Increment counters
    todo_count++;
    y_offset += TODO_HEIGHT;
    
    // Clear the input field
    LG_SetWidgetText(new_todo_field, "");
    
    // Update status
    update_status();
}

/**
 * @brief Clear all completed todos
 */
void clear_completed() {
    // Start from the end to avoid index shifting issues
    for (int i = todo_count - 1; i >= 0; i--) {
        if (todos[i].completed) {
            delete_todo(i);
        }
    }
}

/**
 * @brief Find todo index by widget
 */
int find_todo_by_widget(LG_WidgetHandle widget) {
    for (int i = 0; i < todo_count; i++) {
        if (todos[i].checkbox == widget || 
            todos[i].label == widget || 
            todos[i].delete_button == widget) {
            return i;
        }
    }
    return -1;
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
                
                if (widget == add_button) {
                    add_todo();
                } else if (widget == clear_completed_button) {
                    clear_completed();
                } else {
                    // Check if it's a todo item
                    int index = find_todo_by_widget(widget);
                    if (index >= 0) {
                        if (widget == todos[index].checkbox) {
                            // Toggle completion
                            toggle_todo(index);
                            
                            // Update checkbox text
                            if (todos[index].completed) {
                                LG_SetWidgetText(todos[index].checkbox, "[✓]");
                            } else {
                                LG_SetWidgetText(todos[index].checkbox, "[ ]");
                            }
                        } else if (widget == todos[index].delete_button) {
                            // Delete todo
                            delete_todo(index);
                        }
                    }
                }
            }
            break;
            
        case LG_EVENT_KEY:
            // Handle Enter key to add todo
            if (event->data.key.key_code == 13 && event->data.key.pressed) {
                add_todo();
            }
            break;
    }
}

/**
 * @brief Update the task count label
 */
void update_task_count() {
    char label_text[64];
    int completed = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].completed) {
            completed++;
        }
    }
    
    snprintf(label_text, sizeof(label_text), "Tasks: %d, Completed: %d", task_count, completed);
    LG_SetWidgetText(status_label, label_text);
}

/**
 * @brief Add a new task
 */
void add_task(const char* text) {
    if (task_count >= MAX_TASKS) {
        printf("Cannot add more tasks, maximum reached.\n");
        return;
    }
    
    if (strlen(text) == 0) {
        printf("Cannot add empty task.\n");
        return;
    }
    
    // Calculate position for new task
    int y = start_y + task_count * (task_height + task_spacing);
    
    // Create checkbox
    tasks[task_count].checkbox = LG_CreateButton(window, "[ ]", 20, y, 30, task_height);
    
    // Create task text label
    tasks[task_count].text_label = LG_CreateLabel(window, text, 60, y, 350, task_height);
    
    // Create delete button
    tasks[task_count].delete_button = LG_CreateButton(window, "X", 420, y, 30, task_height);
    
    // Store task text and mark as not completed
    strncpy(tasks[task_count].text, text, MAX_TASK_TEXT - 1);
    tasks[task_count].text[MAX_TASK_TEXT - 1] = '\0';
    tasks[task_count].completed = false;
    
    task_count++;
    update_task_count();
}

/**
 * @brief Toggle task completion status
 */
void toggle_task(int index) {
    if (index < 0 || index >= task_count) {
        return;
    }
    
    tasks[index].completed = !tasks[index].completed;
    
    if (tasks[index].completed) {
        LG_SetWidgetText(tasks[index].checkbox, "[X]");
        // Optional: strike through or change color of completed tasks
        LG_SetWidgetTextColor(tasks[index].text_label, LG_CreateColor(128, 128, 128, 255));
    } else {
        LG_SetWidgetText(tasks[index].checkbox, "[ ]");
        LG_SetWidgetTextColor(tasks[index].text_label, LG_CreateColor(0, 0, 0, 255));
    }
    
    update_task_count();
}

/**
 * @brief Delete a task
 */
void delete_task(int index) {
    if (index < 0 || index >= task_count) {
        return;
    }
    
    // Destroy widgets for the deleted task
    LG_DestroyWidget(tasks[index].checkbox);
    LG_DestroyWidget(tasks[index].text_label);
    LG_DestroyWidget(tasks[index].delete_button);
    
    // Shift all tasks after the deleted one
    for (int i = index; i < task_count - 1; i++) {
        tasks[i] = tasks[i + 1];
        
        // Update position of the moved task
        int y = start_y + i * (task_height + task_spacing);
        LG_SetWidgetPosition(tasks[i].checkbox, 20, y);
        LG_SetWidgetPosition(tasks[i].text_label, 60, y);
        LG_SetWidgetPosition(tasks[i].delete_button, 420, y);
    }
    
    task_count--;
    update_task_count();
}

/**
 * @brief Clear all tasks
 */
void clear_all_tasks() {
    // Destroy all task widgets
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].checkbox) LG_DestroyWidget(tasks[i].checkbox);
        if (tasks[i].text_label) LG_DestroyWidget(tasks[i].text_label);
        if (tasks[i].delete_button) LG_DestroyWidget(tasks[i].delete_button);
        
        // Reset handles to NULL to prevent double-free issues
        tasks[i].checkbox = NULL;
        tasks[i].text_label = NULL;
        tasks[i].delete_button = NULL;
    }
    
    // Reset task count
    task_count = 0;
    update_task_count();
}

int main(void) {
    // Initialize LightGUI
    if (!LG_Initialize()) {
        fprintf(stderr, "Failed to initialize LightGUI\n");
        return 1;
    }
    
    // Create window
    window = LG_CreateWindow("Todo List Application", WINDOW_WIDTH, WINDOW_HEIGHT, true);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        LG_Terminate();
        return 1;
    }
    
    // Initialize colors
    title_color = LG_CreateColor(0, 0, 150, 255);     // Dark blue
    completed_color = LG_CreateColor(100, 100, 100, 255); // Gray
    normal_color = LG_CreateColor(0, 0, 0, 255);      // Black
    delete_color = LG_CreateColor(200, 0, 0, 255);    // Red
    
    // Set event callback
    LG_SetEventCallback(window, event_callback, NULL);
    
    // Create title
    title_label = LG_CreateLabel(window, "Todo List", 20, 20, WINDOW_WIDTH - 40, 40);
    LG_SetWidgetTextColor(title_label, title_color);
    
    // Create new todo field and add button
    new_todo_field = LG_CreateTextField(window, "", 20, 70, 350, 30);
    add_button = LG_CreateButton(window, "Add", 380, 70, 100, 30);
    
    // Create clear completed button
    clear_completed_button = LG_CreateButton(window, "Clear Completed", 20, WINDOW_HEIGHT - 70, 200, 30);
    
    // Create status label
    status_label = LG_CreateLabel(window, "0 items, 0 completed", 20, WINDOW_HEIGHT - 30, WINDOW_WIDTH - 40, 20);
    
    // Show window
    LG_ShowWindow(window);
    
    // Run event loop
    LG_Run();
    
    // Clean up
    clear_all_tasks(); // Ensure all task widgets are properly destroyed
    LG_DestroyWindow(window);
    LG_Terminate();
    
    return 0;
} 