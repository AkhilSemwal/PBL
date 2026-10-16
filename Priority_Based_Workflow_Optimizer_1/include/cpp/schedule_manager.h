// schedule_manager.h
// Owner: Piyush
//
// Holds the timetable of the day as a list of periods. A period is either
//   CLASS - a lecture or a lab; we never put tasks here, or
//   FREE  - free time, where tasks can be done later.
// Every FREE period becomes one free slot (the TimeSlot type from task.h).
// Written in C style: structures, fixed arrays and plain functions.
//
// Times are stored as minutes since midnight (09:30 -> 570).
// A period's end time is not part of it, so 10:00-11:00 and 11:00-12:00
// only touch and do not clash. Gaps that are not written in the file
// (tea or lunch break) are not used.
//
// schedule.txt has one period per line, 6 fields separated by '|':
//   PeriodID|Start|End|Name|Type|LocationID
//   e.g.  3|10:10|11:05|Advanced Probability for AI (TCS 368)|CLASS|1
//         4|11:05|12:00|Free Period|FREE|-
//   Type is CLASS or FREE. A FREE period uses "-" as its location.
//   Times can be 24-hour (13:50) or 12-hour with AM/PM (01:50 PM).

#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include "task.h"              // TimeSlot, MAX_SLOTS and MAX_NAME_LEN
#include "location_graph.h"    // only needed to print building names

// The most periods we keep for one day.
#define MAX_PERIODS MAX_SLOTS

// The two kinds of period.
typedef enum PeriodType
{
    PERIOD_CLASS = 0,
    PERIOD_FREE = 1
} PeriodType;

// What happened when we tried to add a period.
typedef enum ScheduleStatus
{
    SM_OK = 0,
    SM_ERR_INVALID_TIME,   // start is not before end, or the time is outside the day
    SM_ERR_OVERLAP,        // it clashes with a period we already have
    SM_ERR_DUPLICATE_ID,   // another period already uses this ID
    SM_ERR_INVALID,        // bad ID or name, or the wrong location for its type
    SM_ERR_FULL            // we already have MAX_PERIODS periods
} ScheduleStatus;

// One period of the timetable.
typedef struct Period
{
    int id;
    int startMinute;
    int endMinute;
    char name[MAX_NAME_LEN];
    PeriodType type;
    int locationId;        // the building of a class; NO_LOCATION for a free period
} Period;

// The whole timetable.
typedef struct ScheduleManager
{
    Period periods[MAX_PERIODS];     // always kept in order of start time
    int periodCount;
    TimeSlot freeSlots[MAX_SLOTS];   // made again after every new period
    int slotCount;
} ScheduleManager;

// Start with an empty timetable.
void sm_init(ScheduleManager* manager);

// Adds one period after checking it. If it is fine, the periods are sorted
// again and the free slots are made again.
ScheduleStatus sm_add_period(ScheduleManager* manager, const Period* period);

// Returns 1 if the two periods share at least one minute.
int sm_periods_overlap(const Period* first, const Period* second);

// Puts the periods in order of start time, using heapSort() from heap.h.
void sm_sort_periods(ScheduleManager* manager);

// Makes the list of free slots from the (sorted) periods.
// Returns how many free slots there are.
int sm_build_free_slots(ScheduleManager* manager);

// Small getters.
int sm_period_count(const ScheduleManager* manager);
int sm_slot_count(const ScheduleManager* manager);
const Period* sm_get_period(const ScheduleManager* manager, int index);   // NULL for a wrong index
const TimeSlot* sm_get_slots(const ScheduleManager* manager);
int sm_total_free_minutes(const ScheduleManager* manager);                // all free slots added up

// Prints all periods, then the free slots. If locations is NULL we print
// "Location <ID>" instead of the building name.
void sm_display_timetable(const ScheduleManager* manager, const LocationGraph* locations);

// Reads schedule.txt. Returns how many periods were added, or -1 if the
// file could not be opened. *errorCount tells how many lines were rejected.
int sm_load_from_file(ScheduleManager* manager, const char* path, int* errorCount);

// Turns a status code into a short message we can print.
const char* sm_status_message(ScheduleStatus status);

// Coming in Phase 3: sm_delete_period(), sm_next_id() and sm_save_to_file(),
// so periods can be added and deleted from the menu and saved.

#endif // SCHEDULE_MANAGER_H
