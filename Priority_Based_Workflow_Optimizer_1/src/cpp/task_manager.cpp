// task_manager.cpp
// Owner: Piyush
//
// The task list: creating it, checking tasks, finding them, printing
// them and reading them from tasks.txt.
// Written in C style: a dynamic array of structures and plain functions.

#include "task_manager.h"
#include "task.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// One line of tasks.txt has exactly this many '|'-separated fields.
#define TASK_FIELD_COUNT 7

int tm_init(TaskManager* manager)
{
    manager->count = 0;
    manager->capacity = 0;

    // One block of memory big enough for the largest list we allow.
    manager->tasks = (TaskData*)malloc((size_t)MAX_TASKS * sizeof(TaskData));
    if (manager->tasks == NULL) {
        return 0;
    }
    manager->capacity = MAX_TASKS;
    return 1;
}

void tm_free(TaskManager* manager)
{
    free(manager->tasks);       // free(NULL) does nothing, so a second call is safe
    manager->tasks = NULL;      // no dangling pointer left behind
    manager->count = 0;
    manager->capacity = 0;
}

const char* tm_validation_error(const TaskData* task)
{
    // We check the rules one by one and stop at the first problem,
    // so the user gets one clear message.
    if (task->id < 1) {
        return "task ID must be 1 or more";
    }
    if (task->name[0] == '\0') {
        return "task name is empty";
    }
    // '|' separates the fields in the file, so it cannot be part of a name.
    if (strchr(task->name, FIELD_SEPARATOR) != NULL) {
        return "task name cannot contain the '|' character";
    }
    if (task->durationMinutes < MIN_TASK_DURATION || task->durationMinutes > MAX_TASK_DURATION) {
        return "duration must be between 1 and 600 minutes";
    }
    if (task->priority < MIN_PRIORITY || task->priority > MAX_PRIORITY) {
        return "priority must be between 1 and 10";
    }
    if (task->locationId < 1) {
        return "location ID must be 1 or more";
    }
    if (task->prerequisiteCount < 0 || task->prerequisiteCount > MAX_PREREQUISITES) {
        return "too many prerequisites (maximum 5)";
    }
    // Every prerequisite must be a real ID, not the task itself, and not repeated.
    for (int first = 0; first < task->prerequisiteCount; first++) {
        int prerequisite = task->prerequisiteIds[first];
        if (prerequisite < 1) {
            return "prerequisite IDs must be 1 or more";
        }
        if (prerequisite == task->id) {
            return "a task cannot be its own prerequisite";
        }
        for (int second = first + 1; second < task->prerequisiteCount; second++) {
            if (task->prerequisiteIds[second] == prerequisite) {
                return "the same prerequisite is listed twice";
            }
        }
    }
    if (task->isCompleted != 0 && task->isCompleted != 1) {
        return "status must be PENDING or DONE";
    }
    return NULL;   // no problem found
}

int tm_find_index(const TaskManager* manager, int taskId)
{
    // Linear search - at most 100 tasks, so this is quick enough.
    for (int index = 0; index < manager->count; index++) {
        if (manager->tasks[index].id == taskId) {
            return index;
        }
    }
    return -1;
}

const TaskData* tm_find_task(const TaskManager* manager, int taskId)
{
    int index = tm_find_index(manager, taskId);
    if (index == -1) {
        return NULL;
    }
    return &manager->tasks[index];
}

const TaskData* tm_get_task_at(const TaskManager* manager, int index)
{
    // Check the position first so we never read outside the array.
    if (index < 0 || index >= manager->count) {
        return NULL;
    }
    return &manager->tasks[index];
}

TaskManagerStatus tm_add_task(TaskManager* manager, const TaskData* task)
{
    if (manager->tasks == NULL) {
        return TM_ERR_NO_MEMORY;   // tm_init was not called, or it failed
    }
    if (tm_validation_error(task) != NULL) {
        return TM_ERR_INVALID;
    }
    if (tm_find_index(manager, task->id) != -1) {
        return TM_ERR_DUPLICATE_ID;
    }
    if (manager->count >= manager->capacity) {
        return TM_ERR_FULL;
    }
    manager->tasks[manager->count] = *task;   // store our own copy at the end
    manager->count++;
    return TM_OK;
}

int tm_count(const TaskManager* manager)
{
    return manager->count;
}

int tm_count_pending(const TaskManager* manager)
{
    int pending = 0;
    for (int index = 0; index < manager->count; index++) {
        if (!manager->tasks[index].isCompleted) {
            pending++;
        }
    }
    return pending;
}

// The column titles of the task table.
static void print_task_header(void)
{
    printf("  %-4s %-32s %4s %5s  %-18s %-10s %s\n",
           "ID", "Name", "Pri", "Min", "Location", "Needs", "Status");
    printf("  ---------------------------------------------------------------------------------------\n");
}

// One row of the task table.
static void print_task_row(const TaskData* task, const LocationGraph* locations)
{
    // Build the "Needs" column, e.g. "T1" or "T1,T4", or "-" if there is nothing.
    char needs[32];
    needs[0] = '\0';
    if (task->prerequisiteCount == 0) {
        snprintf(needs, sizeof(needs), "-");
    }
    for (int position = 0; position < task->prerequisiteCount; position++) {
        char part[8];
        snprintf(part, sizeof(part), "%sT%d", position > 0 ? "," : "", task->prerequisiteIds[position]);
        if (strlen(needs) + strlen(part) < sizeof(needs)) {   // only add it if it still fits
            strcat(needs, part);
        }
    }

    // Show the building name if we have the map, otherwise just its ID.
    const char* locationName = "?";
    char locationText[24];
    if (locations != NULL) {
        locationName = lg_name_of(locations, task->locationId);
        snprintf(locationText, sizeof(locationText), "%s", locationName);
    } else {
        snprintf(locationText, sizeof(locationText), "Location %d", task->locationId);
    }

    // %-18.18s keeps the location column 18 characters wide, even for long names.
    printf("  T%-3d %-32s %4d %5d  %-18.18s %-10s %s\n",
           task->id, task->name, task->priority, task->durationMinutes,
           locationText, needs, task->isCompleted ? "DONE" : "PENDING");
}

void tm_display_all(const TaskManager* manager, const LocationGraph* locations)
{
    printf("All tasks (%d)\n", manager->count);
    if (manager->count == 0) {
        printf("  (no tasks loaded)\n");
        return;
    }
    print_task_header();
    for (int index = 0; index < manager->count; index++) {
        print_task_row(&manager->tasks[index], locations);
    }
}

void tm_display_pending(const TaskManager* manager, const LocationGraph* locations)
{
    int pending = tm_count_pending(manager);
    printf("Pending tasks (%d)\n", pending);
    if (pending == 0) {
        printf("  (nothing pending)\n");
        return;
    }
    print_task_header();
    for (int index = 0; index < manager->count; index++) {
        if (!manager->tasks[index].isCompleted) {   // skip the tasks already DONE
            print_task_row(&manager->tasks[index], locations);
        }
    }
}

// Reads the prerequisite field: "-" (none) or a list like "1,4,7".
// Returns 1 if it is fine, 0 if it is not.
static int parse_prerequisites(char* text, TaskData* task)
{
    task->prerequisiteCount = 0;
    if (strcmp(text, "-") == 0 || text[0] == '\0') {
        return 1;   // this task needs nothing
    }

    // Same splitting idea as the main line, but with ',' instead of '|'.
    char* parts[MAX_PREREQUISITES];
    int partCount = pu_split(text, ',', parts, MAX_PREREQUISITES);
    if (partCount > MAX_PREREQUISITES) {
        return 0;   // more than 5 prerequisites
    }
    for (int position = 0; position < partCount; position++) {
        int prerequisiteId = 0;
        if (!pu_parse_int(parts[position], &prerequisiteId)) {
            return 0;   // e.g. "1,,2" or "1,a"
        }
        task->prerequisiteIds[position] = prerequisiteId;
        task->prerequisiteCount++;
    }
    return 1;
}

// Turns the 7 fields of one line into a TaskData.
// Returns NULL if the line is fine, otherwise a message saying what is wrong.
static const char* parse_task_fields(char** fields, TaskData* task)
{
    memset(task, 0, sizeof(TaskData));   // start clean, so no old values stay behind

    // The field order follows the file: ID, Name, Priority, Duration, Location,
    // Prerequisites, Status.
    if (!pu_parse_int(fields[0], &task->id)) {
        return "ID is not a number";
    }
    if (!pu_copy_text(task->name, MAX_NAME_LEN, fields[1])) {
        return "name is too long (max 63 characters)";
    }
    if (!pu_parse_int(fields[2], &task->priority)) {
        return "priority is not a number";
    }
    if (!pu_parse_int(fields[3], &task->durationMinutes)) {
        return "duration is not a number";
    }
    if (!pu_parse_int(fields[4], &task->locationId)) {
        return "location ID is not a number";
    }
    if (!parse_prerequisites(fields[5], task)) {
        return "prerequisites must be '-' or IDs like 1,4 (max 5)";
    }

    // "DONE" and "done" both work, because we ignore capital letters.
    if (pu_equals_ignore_case(fields[6], "DONE")) {
        task->isCompleted = 1;
    } else if (pu_equals_ignore_case(fields[6], "PENDING")) {
        task->isCompleted = 0;
    } else {
        return "status must be PENDING or DONE";
    }

    // The numbers are readable - now check that they follow the rules.
    return tm_validation_error(task);
}

int tm_load_from_file(TaskManager* manager, const char* path, int* errorCount)
{
    *errorCount = 0;
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("  [ERROR] Cannot open tasks file: %s\n", path);
        return -1;
    }

    char line[LINE_BUFFER_SIZE];
    int lineNumber = 0;
    int loaded = 0;
    int tooLong = 0;

    // Read one line at a time. A bad line is reported and skipped; the
    // rest of the file is still read.
    while (pu_read_line(file, line, LINE_BUFFER_SIZE, &tooLong)) {
        lineNumber++;
        if (tooLong) {
            printf("  [WARN] %s line %d: line too long, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }
        if (pu_is_skippable(line)) {
            continue;   // empty line or a '#' comment
        }

        char* fields[TASK_FIELD_COUNT];
        int fieldCount = pu_split(line, FIELD_SEPARATOR, fields, TASK_FIELD_COUNT);
        if (fieldCount != TASK_FIELD_COUNT) {
            printf("  [WARN] %s line %d: expected 7 fields, found %d, skipped\n",
                   path, lineNumber, fieldCount);
            (*errorCount)++;
            continue;
        }

        // Step 1: can the fields be read, and do they follow the rules?
        TaskData task;
        const char* problem = parse_task_fields(fields, &task);
        if (problem != NULL) {
            printf("  [WARN] %s line %d: %s, skipped\n", path, lineNumber, problem);
            (*errorCount)++;
            continue;
        }

        // Step 2: can it go into the list (no duplicate ID, list not full)?
        TaskManagerStatus status = tm_add_task(manager, &task);
        if (status != TM_OK) {
            printf("  [WARN] %s line %d: task %d rejected (%s)\n",
                   path, lineNumber, task.id, tm_status_message(status));
            (*errorCount)++;
            continue;
        }
        loaded++;
    }

    fclose(file);
    return loaded;
}

const char* tm_status_message(TaskManagerStatus status)
{
    switch (status) {
    case TM_OK:
        return "OK";
    case TM_ERR_DUPLICATE_ID:
        return "a task with this ID already exists";
    case TM_ERR_INVALID:
        return "task data is invalid";
    case TM_ERR_FULL:
        return "task list is full";
    case TM_ERR_NOT_FOUND:
        return "task not found";
    case TM_ERR_HAS_DEPENDENTS:
        return "another task depends on it";
    case TM_ERR_NO_MEMORY:
        return "out of memory";
    }
    return "unknown status";
}
