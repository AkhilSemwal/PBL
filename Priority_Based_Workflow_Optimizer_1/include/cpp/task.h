/*
 * task.h
 * Owner: Akhil (common data model)
 *
 * This is the ONE place where the project's shared data types are defined.
 * Every other module includes this file instead of making its own copy.
 *
 * Part 1 (DS with C style) : constants, two plain structures (TaskData,
 *                            TimeSlot) and small text helpers (pu_...)
 *                            used by every file loader.
 * Part 2 (OOP with C++)    : the Task class.
 *
 * Phase 3 will add here: the plan result types (PlannedTask, SlotUsage,
 * DeferredTask, ScheduleResult), SchedulingInput and the abstract base
 * class Scheduler that GreedyScheduler and KnapsackScheduler inherit from.
 */
#ifndef TASK_H
#define TASK_H

/* ===================== Part 1: plain data (C style) ===================== */

/* Longest name (task, location, activity) including the final '\0'. */
#define MAX_NAME_LEN 64

/* A task may list at most this many prerequisite tasks. */
#define MAX_PREREQUISITES 5

/* Upper limits for the fixed-size tables used in the project. */
#define MAX_TASKS 100
#define MAX_SLOTS 50
#define MAX_LOCATIONS 50

/* Value used when "no location" is meant. Real location IDs are >= 1. */
#define NO_LOCATION (-1)

/* Valid priority range. A HIGHER number means a MORE important task. */
#define MIN_PRIORITY 1
#define MAX_PRIORITY 10

/* Valid task duration range in minutes. */
#define MIN_TASK_DURATION 1
#define MAX_TASK_DURATION 600

/* One pending (or completed) piece of work, exactly as stored in tasks.txt. */
typedef struct TaskData {
    int id;                                   /* unique task ID, >= 1           */
    char name[MAX_NAME_LEN];                  /* human readable name            */
    int durationMinutes;                      /* how long the task takes        */
    int priority;                             /* 1..10, higher = more important */
    int locationId;                           /* where the task must be done    */
    int prerequisiteCount;                    /* number of used entries below   */
    int prerequisiteIds[MAX_PREREQUISITES];   /* tasks that must finish first   */
    int isCompleted;                          /* 1 = already done, 0 = pending  */
} TaskData;

/*
 * One FREE period of the timetable, given to the schedulers.
 * All times are "minutes since midnight", e.g. 09:30 -> 570.
 *
 * fromLocationId: location of the CLASS right before this free period.
 *                 NO_LOCATION if the period before it was also free (or the
 *                 day starts with it). Then the scheduler uses the place where
 *                 the student finished the previous free period.
 * toLocationId:   location of the CLASS right after this free period.
 *                 NO_LOCATION if the next period is also free (or the day
 *                 ends), so no walk to a class is needed inside this slot.
 */
typedef struct TimeSlot {
    int slotId;                               /* 1, 2, 3 ... in time order      */
    int startMinute;
    int endMinute;
    int durationMinutes;                      /* endMinute - startMinute        */
    int fromLocationId;                       /* see comment above              */
    int toLocationId;                         /* see comment above              */
    char previousActivity[MAX_NAME_LEN];      /* period before ("-" if none)    */
    char nextActivity[MAX_NAME_LEN];          /* period after  ("-" if none)    */
} TimeSlot;

/* ---------- text helpers for reading the data files (no std::string) ---------- */

#include <cstdio>   /* FILE */

/* Size of the buffer used to read one line from a data file. */
#define LINE_BUFFER_SIZE 256

/* Field separator used in every data file. */
#define FIELD_SEPARATOR '|'

/*
 * Reads one line into buffer (newline removed).
 * Returns 1 if a line was read, 0 at end of file.
 * If the line did not fit, the rest of it is skipped and *lineTooLong = 1.
 */
int pu_read_line(FILE* file, char* buffer, int bufferSize, int* lineTooLong);

/* Removes spaces, tabs and '\r' from both ends of text (in place). */
void pu_trim(char* text);

/* Returns 1 for an empty line or a comment line starting with '#'. */
int pu_is_skippable(const char* line);

/*
 * Splits line at every separator (in place) and stores pointers to the
 * trimmed fields. Returns the number of fields found (it can be larger than
 * maxFields, which tells the caller that the record has too many fields).
 */
int pu_split(char* line, char separator, char** fields, int maxFields);

/* Strict integer parse: optional sign and digits only. Returns 1 on success. */
int pu_parse_int(const char* text, int* value);

/* Parses "HH:MM" (00:00 .. 23:59) into minutes since midnight. Returns 1 on success. */
int pu_parse_time(const char* text, int* minutes);

/* Writes minutes since midnight as "HH:MM" into buffer (needs 6 chars). */
void pu_format_time(int minutes, char* buffer, int bufferSize);

/*
 * Copies source into destination only if it fits completely.
 * Returns 1 on success, 0 if source is too long (destination becomes "").
 */
int pu_copy_text(char* destination, int destinationSize, const char* source);

/* Case-insensitive comparison of two texts. Returns 1 if equal. */
int pu_equals_ignore_case(const char* first, const char* second);

/* ===================== Part 2: classes (OOP with C++) ===================== */

#include <iostream>

/*
 * Task - an object-oriented, read-only view of one TaskData record.
 *
 * Encapsulation: the record is private. Other classes can read it through
 * getters but cannot change it. Changes (add, update, complete, delete)
 * always go through TaskManager, which validates them first.
 */
class Task {
public:
    Task();
    explicit Task(const TaskData& record);

    int getId() const;
    const char* getName() const;
    int getDuration() const;
    int getPriority() const;
    int getLocationId() const;
    int getPrerequisiteCount() const;
    int getPrerequisiteId(int position) const;   /* returns -1 if position is invalid */
    bool isCompleted() const;
    bool hasPrerequisite(int taskId) const;

    /*
     * Greedy ranking rule used by the whole project:
     *   1. higher priority first
     *   2. if equal, shorter duration first
     *   3. if still equal, lower task ID first
     */
    bool ranksHigherThan(const Task& other) const;

    /* "a > b" means "a should be chosen before b" (same as ranksHigherThan). */
    bool operator>(const Task& other) const;

    /* Two Task objects are equal when they describe the same task ID. */
    bool operator==(const Task& other) const;

private:
    TaskData data;
};

/* Prints one task on a single line, e.g. "T3 Submit Lab Record (15 min, P10)". */
std::ostream& operator<<(std::ostream& out, const Task& task);

/*
 * ---------------------------------------------------------------------
 * Phase 3: PlannedTask, SlotUsage, DeferredTask, SchedulingInput,
 *          ScheduleResult and the abstract class Scheduler go here.
 * ---------------------------------------------------------------------
 */

#endif /* TASK_H */
