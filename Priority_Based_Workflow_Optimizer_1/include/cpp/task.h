// task.h
// Owner: Akhil
//
// This is the "dictionary" of our project. It defines what a task and a
// free time slot look like, so every other file uses exactly the same
// definition instead of making its own copy.
//
// Part 1 (C style)  : limits, two plain structures (TaskData, TimeSlot)
//                     and small text helpers (pu_...) that every file
//                     reader uses.
// Part 2 (C++ / OOP): the Task class.
//
// In Phase 3 we will also add the plan result types and the abstract
// Scheduler class here.

#ifndef TASK_H
#define TASK_H

// ============================ Part 1: plain data ============================

// Longest name we store (task, building, period), counting the '\0' at the end.
// So a name can have at most 63 characters.
#define MAX_NAME_LEN 64

// A task can need at most this many other tasks to be finished first.
#define MAX_PREREQUISITES 5

// Size limits for our fixed arrays.
#define MAX_TASKS 100
#define MAX_SLOTS 50
#define MAX_LOCATIONS 50

// Used when we mean "no building". Real building IDs start from 1.
#define NO_LOCATION (-1)

// Allowed priority values. A bigger number means a more important task.
#define MIN_PRIORITY 1
#define MAX_PRIORITY 10

// Allowed task length in minutes.
#define MIN_TASK_DURATION 1
#define MAX_TASK_DURATION 600

// One task, exactly as it is written in tasks.txt.
typedef struct TaskData
{
    int id;                                   // unique number, 1 or more
    char name[MAX_NAME_LEN];                  // e.g. "Submit Lab Record"
    int durationMinutes;                      // how long it takes
    int priority;                             // 1..10, higher = more important
    int locationId;                           // the building where it is done
    int prerequisiteCount;                    // how many entries below are used
    int prerequisiteIds[MAX_PREREQUISITES];   // tasks that must be finished first
    int isCompleted;                          // 1 = already done, 0 = still pending
} TaskData;

// One free period of the day.
// All times are stored as "minutes since midnight", e.g. 09:30 -> 570,
// because numbers are easy to compare and subtract.
//
// fromLocationId: where the class just before this free period was held.
//                 It is NO_LOCATION if the period before was also free (or
//                 if the day starts with this free period).
// toLocationId:   where the class just after this free period is held.
//                 It is NO_LOCATION if the next period is also free (or the
//                 day ends), so nobody has to walk to a class.
typedef struct TimeSlot
{
    int slotId;                               // 1, 2, 3 ... in time order
    int startMinute;
    int endMinute;
    int durationMinutes;                      // endMinute - startMinute
    int fromLocationId;                       // see the note above
    int toLocationId;                         // see the note above
    char previousActivity[MAX_NAME_LEN];      // name of the period before ("-" if none)
    char nextActivity[MAX_NAME_LEN];          // name of the period after  ("-" if none)
} TimeSlot;

// ------------------- text helpers for reading our data files -------------------
// We are not allowed to use std::string, so we work with char arrays.

#include <cstdio>   // FILE

// Size of the buffer we read one line of a file into.
#define LINE_BUFFER_SIZE 256

// Every data file separates its fields with this character.
#define FIELD_SEPARATOR '|'

// Reads one line into buffer and removes the '\n' at the end.
// Returns 1 if a line was read, 0 when the file is finished.
// If the line was too long for the buffer, the rest of it is skipped
// and *lineTooLong is set to 1, so the caller can report it.
int pu_read_line(FILE* file, char* buffer, int bufferSize, int* lineTooLong);

// Removes spaces, tabs and line-end characters from both ends of text.
// The text is changed in place.
void pu_trim(char* text);

// Returns 1 if the line is empty or is a comment (starts with '#').
int pu_is_skippable(const char* line);

// Cuts the line at every separator and makes fields[i] point at each part
// (each part is trimmed). The line itself is changed: every separator becomes '\0'.
// Returns how many parts the line has. If that is more than maxFields,
// only the first maxFields parts are stored - the caller uses the count
// to see that the line has too many fields.
int pu_split(char* line, char separator, char** fields, int maxFields);

// Turns text like "42" or "-7" into a number. Only an optional sign and
// digits are allowed ("4a" is rejected). Returns 1 on success, 0 if not.
int pu_parse_int(const char* text, int* value);

// Turns a time into minutes since midnight. Two formats are accepted:
//   24-hour: "13:50"
//   12-hour: "01:50 PM" or "08:00 AM" (one space, then AM or PM)
// Returns 1 on success, 0 if the text is not a valid time.
int pu_parse_time(const char* text, int* minutes);

// The opposite: writes minutes since midnight as "HH:MM" (needs 6 chars).
void pu_format_time(int minutes, char* buffer, int bufferSize);

// Copies source into destination only if the whole text fits.
// Returns 1 on success. If it is too long, destination becomes "" and 0 is returned.
int pu_copy_text(char* destination, int destinationSize, const char* source);

// Compares two texts without caring about capital letters ("Library" == "library").
// Returns 1 if they are the same.
int pu_equals_ignore_case(const char* first, const char* second);

// ============================ Part 2: the Task class ============================

#include <iostream>

// Task gives an object-oriented, read-only view of one TaskData record.
//
// The record is private (encapsulation): other code can read it through
// the getters below but cannot change it. Adding or changing tasks is
// done by the task manager, which checks the data first.
class Task
{
public:
    Task();                                // an empty task (location = NO_LOCATION)
    explicit Task(const TaskData& record); // a task made from a record of tasks.txt

    int getId() const;
    const char* getName() const;
    int getDuration() const;
    int getPriority() const;
    int getLocationId() const;
    int getPrerequisiteCount() const;
    int getPrerequisiteId(int position) const;   // gives -1 for a wrong position
    bool isCompleted() const;
    bool hasPrerequisite(int taskId) const;      // does this task need taskId?

    // The order in which we prefer tasks, used everywhere in the project:
    //   1. higher priority first
    //   2. if the priority is the same, the shorter task first
    //   3. if that is also the same, the lower ID first
    bool ranksHigherThan(const Task& other) const;

    // "a > b" means "a should be picked before b" (same rule as above).
    bool operator>(const Task& other) const;

    // Two Task objects are equal if they are the same task (same ID).
    bool operator==(const Task& other) const;

private:
    TaskData data;
};

// Lets us print a task with cout, e.g. "T3 Submit Lab Record (15 min, P10)".
std::ostream& operator<<(std::ostream& out, const Task& task);

// Phase 3: PlannedTask, SlotUsage, DeferredTask, SchedulingInput,
// ScheduleResult and the abstract Scheduler class will be added here.

#endif // TASK_H
