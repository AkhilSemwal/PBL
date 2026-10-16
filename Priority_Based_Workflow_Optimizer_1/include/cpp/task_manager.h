/*
 * task_manager.h
 * Owner: Piyush
 * Style: DS with C (dynamic array of structures, functions)
 *
 * Stores every TaskData record in one dynamically allocated array
 * and provides load / validate / find / display.
 * (Phase 3 will add: update, delete, mark completed, save to file.)
 *
 * Task IDs: every task has a unique ID >= 1, taken from tasks.txt.
 * tm_add_task() rejects a duplicate ID, so two tasks can never share one.
 *
 * tasks.txt format (one task per line, '|' separated, 7 fields):
 *   ID|Name|Priority|DurationMinutes|LocationID|Prerequisites|Status
 *   Prerequisites: "-" for none, or IDs separated by commas, e.g. "1,4"
 *   Status: PENDING or DONE
 */
#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "task.h"              /* TaskData (the one task definition) */
#include "location_graph.h"    /* only for printing location names    */

typedef enum TaskManagerStatus {
    TM_OK = 0,
    TM_ERR_DUPLICATE_ID,
    TM_ERR_INVALID,
    TM_ERR_FULL,
    TM_ERR_NOT_FOUND,
    TM_ERR_HAS_DEPENDENTS,
    TM_ERR_NO_MEMORY
} TaskManagerStatus;

typedef struct TaskManager {
    TaskData* tasks;   /* malloc'ed array of "capacity" records */
    int count;
    int capacity;
} TaskManager;

/* Allocates room for MAX_TASKS tasks. Returns 1 on success. */
int tm_init(TaskManager* manager);

/* Releases the array. Safe to call more than once. */
void tm_free(TaskManager* manager);

/*
 * Checks one record on its own (ranges, name, prerequisite list).
 * Returns NULL if valid, otherwise a short explanation.
 */
const char* tm_validation_error(const TaskData* task);

TaskManagerStatus tm_add_task(TaskManager* manager, const TaskData* task);

/* Returns the index of taskId, or -1. */
int tm_find_index(const TaskManager* manager, int taskId);

/* Returns a pointer to the record, or NULL. */
const TaskData* tm_find_task(const TaskManager* manager, int taskId);

/* Record at a valid index (0 .. count-1), or NULL. */
const TaskData* tm_get_task_at(const TaskManager* manager, int index);

int tm_count(const TaskManager* manager);
int tm_count_pending(const TaskManager* manager);
/* locations may be NULL; then only location IDs are printed. */
void tm_display_all(const TaskManager* manager, const LocationGraph* locations);
void tm_display_pending(const TaskManager* manager, const LocationGraph* locations);

/* Returns records loaded, or -1 if the file cannot be opened. */
int tm_load_from_file(TaskManager* manager, const char* path, int* errorCount);

const char* tm_status_message(TaskManagerStatus status);


/* Phase 3: tm_update_task(), tm_delete_task(), tm_mark_completed(),
   tm_next_id() and tm_save_to_file() (edit tasks from the menu, Save Data). */

#endif /* TASK_MANAGER_H */
