// task_manager.h
// Owner: Piyush
//
// Keeps all our tasks in one list. It reads tasks.txt line by line,
// checks every line before accepting it, and can find and print tasks.
// Written in C style: an array of structures and plain functions.
//
// Task IDs: every task has its own ID (1 or more), taken from tasks.txt.
// tm_add_task() refuses an ID that is already used, so two tasks can
// never share one.
//
// tasks.txt has one task per line, 7 fields separated by '|':
//   ID|Name|Priority|DurationMinutes|LocationID|Prerequisites|Status
//   e.g.  2|Solve Heap Practice Set|8|35|2|1|PENDING
//   Prerequisites: "-" if there are none, or IDs with commas, e.g. "1,4"
//   Status:        PENDING (still to do) or DONE (already finished)

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "task.h"              // TaskData - the one task structure everybody uses
#include "location_graph.h"    // only needed to print building names

// What happened when we tried to change the task list.
typedef enum TaskManagerStatus
{
    TM_OK = 0,
    TM_ERR_DUPLICATE_ID,     // another task already has this ID
    TM_ERR_INVALID,          // the task data breaks one of the rules
    TM_ERR_FULL,             // we already have MAX_TASKS tasks
    TM_ERR_NOT_FOUND,        // no task with this ID (used by the Phase 3 functions)
    TM_ERR_HAS_DEPENDENTS,   // another task needs this one (used by Phase 3 delete)
    TM_ERR_NO_MEMORY         // the list could not be created
} TaskManagerStatus;

// The task list itself: a dynamic array made with malloc.
typedef struct TaskManager
{
    TaskData* tasks;   // the array of tasks
    int count;         // how many tasks are stored right now
    int capacity;      // how many tasks fit in the array (MAX_TASKS)
} TaskManager;

// Creates an empty list with room for MAX_TASKS tasks.
// Returns 1 on success, 0 if malloc failed.
int tm_init(TaskManager* manager);

// Gives the memory back. Calling it twice does no harm.
void tm_free(TaskManager* manager);

// Checks one task on its own: ID, name, duration, priority, location and
// the prerequisite list. Returns NULL if everything is fine, otherwise a
// short message saying what is wrong.
const char* tm_validation_error(const TaskData* task);

// Adds a copy of the task to the list, after checking it.
TaskManagerStatus tm_add_task(TaskManager* manager, const TaskData* task);

// Where is this task in the list? Returns its index, or -1 if it is not there.
int tm_find_index(const TaskManager* manager, int taskId);

// The task with this ID, or NULL if there is none.
const TaskData* tm_find_task(const TaskManager* manager, int taskId);

// The task at a position in the list (0 .. count-1), or NULL for a wrong position.
const TaskData* tm_get_task_at(const TaskManager* manager, int index);

// How many tasks we have, and how many of them are not done yet.
int tm_count(const TaskManager* manager);
int tm_count_pending(const TaskManager* manager);

// Print the tasks as a table. If locations is NULL we print
// "Location <ID>" instead of the building name.
void tm_display_all(const TaskManager* manager, const LocationGraph* locations);
void tm_display_pending(const TaskManager* manager, const LocationGraph* locations);

// Reads tasks.txt. Returns how many tasks were added, or -1 if the file
// could not be opened. *errorCount tells how many lines were rejected.
int tm_load_from_file(TaskManager* manager, const char* path, int* errorCount);

// Turns a status code into a short message we can print.
const char* tm_status_message(TaskManagerStatus status);

// Coming in Phase 3: tm_update_task(), tm_delete_task(), tm_mark_completed(),
// tm_next_id() and tm_save_to_file(), so tasks can be changed from the menu
// and saved back to tasks.txt.

#endif // TASK_MANAGER_H
