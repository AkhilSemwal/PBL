// schedule_manager.cpp
// Owner: Piyush
//
// The timetable: checking periods, keeping them sorted by time, finding
// the free slots and reading schedule.txt.
// Written in C style: structures, fixed arrays and plain functions.

#include "schedule_manager.h"
#include "heap.h"
#include "task.h"

#include <cstdio>
#include <cstring>

// One line of schedule.txt has exactly this many '|'-separated fields.
#define SCHEDULE_FIELD_COUNT 6

// Minutes in a day. Every period must lie between 0 and 1440.
#define MINUTES_PER_DAY 1440

void sm_init(ScheduleManager* manager)
{
    // The arrays are fixed, so an empty timetable just means "zero items".
    manager->periodCount = 0;
    manager->slotCount = 0;
}

int sm_periods_overlap(const Period* first, const Period* second)
{
    // Two periods clash when each one starts before the other one ends.
    // Example: 10:00-11:00 and 10:30-11:30 -> 600 < 690 and 630 < 660 -> clash.
    //          10:00-11:00 and 11:00-12:00 -> 660 < 660 is false   -> no clash.
    if (first->startMinute < second->endMinute && second->startMinute < first->endMinute) {
        return 1;
    }
    return 0;
}

void sm_sort_periods(ScheduleManager* manager)
{
    HeapNode keys[MAX_PERIODS];
    Period sorted[MAX_PERIODS];
    int count = manager->periodCount;

    // We sort small (key, value) pairs instead of whole periods:
    // key = start time, value = where that period is right now.
    for (int index = 0; index < count; index++) {
        keys[index].key = manager->periods[index].startMinute;
        keys[index].value = index;
    }

    heapSort(keys, count, false);   // false = smallest start time first

    // Copy the periods into a new array in the sorted order, then copy back.
    // (Two periods never start at the same time, because a clash is refused.)
    for (int index = 0; index < count; index++) {
        sorted[index] = manager->periods[keys[index].value];
    }
    for (int index = 0; index < count; index++) {
        manager->periods[index] = sorted[index];
    }
}

int sm_build_free_slots(ScheduleManager* manager)
{
    manager->slotCount = 0;

    for (int index = 0; index < manager->periodCount; index++) {
        const Period* period = &manager->periods[index];
        if (period->type != PERIOD_FREE) {
            continue;   // classes are never used for tasks
        }
        if (manager->slotCount >= MAX_SLOTS) {
            break;
        }

        TimeSlot* slot = &manager->freeSlots[manager->slotCount];
        slot->slotId = manager->slotCount + 1;   // slots are numbered 1, 2, 3 ... in time order
        slot->startMinute = period->startMinute;
        slot->endMinute = period->endMinute;
        slot->durationMinutes = period->endMinute - period->startMinute;

        // Where does the student start? We look at the period just before
        // this one in the list (even if there is a break in between).
        // Only a class tells us a building; if it was also free, we don't know yet.
        slot->fromLocationId = NO_LOCATION;
        pu_copy_text(slot->previousActivity, MAX_NAME_LEN, "-");
        if (index > 0) {
            const Period* before = &manager->periods[index - 1];
            pu_copy_text(slot->previousActivity, MAX_NAME_LEN, before->name);
            if (before->type == PERIOD_CLASS) {
                slot->fromLocationId = before->locationId;
            }
        }

        // Where must the student be when the slot ends? Only if a class comes next.
        slot->toLocationId = NO_LOCATION;
        pu_copy_text(slot->nextActivity, MAX_NAME_LEN, "-");
        if (index + 1 < manager->periodCount) {
            const Period* after = &manager->periods[index + 1];
            pu_copy_text(slot->nextActivity, MAX_NAME_LEN, after->name);
            if (after->type == PERIOD_CLASS) {
                slot->toLocationId = after->locationId;
            }
        }
        manager->slotCount++;
    }
    return manager->slotCount;
}

ScheduleStatus sm_add_period(ScheduleManager* manager, const Period* period)
{
    if (period->id < 1 || period->name[0] == '\0') {
        return SM_ERR_INVALID;
    }
    if (period->type == PERIOD_CLASS && period->locationId < 1) {
        return SM_ERR_INVALID;   // a class has to happen in some building
    }
    if (period->type == PERIOD_FREE && period->locationId != NO_LOCATION) {
        return SM_ERR_INVALID;   // a free period has no fixed building
    }
    if (period->startMinute < 0 || period->endMinute > MINUTES_PER_DAY ||
        period->startMinute >= period->endMinute) {
        return SM_ERR_INVALID_TIME;   // e.g. 14:00-13:00 ends before it starts
    }
    // Compare with every period we already have: same ID, or a clash in time?
    for (int index = 0; index < manager->periodCount; index++) {
        if (manager->periods[index].id == period->id) {
            return SM_ERR_DUPLICATE_ID;
        }
        if (sm_periods_overlap(&manager->periods[index], period)) {
            return SM_ERR_OVERLAP;
        }
    }
    if (manager->periodCount >= MAX_PERIODS) {
        return SM_ERR_FULL;
    }

    // All good: add it, put everything back in time order, and make the
    // free slots again, because a new period can change them.
    manager->periods[manager->periodCount] = *period;
    manager->periodCount++;
    sm_sort_periods(manager);
    sm_build_free_slots(manager);
    return SM_OK;
}

int sm_period_count(const ScheduleManager* manager)
{
    return manager->periodCount;
}

int sm_slot_count(const ScheduleManager* manager)
{
    return manager->slotCount;
}

const Period* sm_get_period(const ScheduleManager* manager, int index)
{
    // Check the position first so we never read outside the array.
    if (index < 0 || index >= manager->periodCount) {
        return NULL;
    }
    return &manager->periods[index];
}

const TimeSlot* sm_get_slots(const ScheduleManager* manager)
{
    return manager->freeSlots;
}

int sm_total_free_minutes(const ScheduleManager* manager)
{
    int total = 0;
    for (int index = 0; index < manager->slotCount; index++) {
        total += manager->freeSlots[index].durationMinutes;
    }
    return total;   // 275 minutes for our Monday timetable
}

// Small helper for printing a building: its name, its ID, or a note when
// we don't know the building (NO_LOCATION).
static const char* location_label(const LocationGraph* locations, int locationId, char* buffer, int size)
{
    if (locationId == NO_LOCATION) {
        snprintf(buffer, (size_t)size, "(wherever you are)");
    } else if (locations != NULL) {
        snprintf(buffer, (size_t)size, "%s", lg_name_of(locations, locationId));
    } else {
        snprintf(buffer, (size_t)size, "Location %d", locationId);
    }
    return buffer;
}

void sm_display_timetable(const ScheduleManager* manager, const LocationGraph* locations)
{
    char startText[8];
    char endText[8];
    char place[MAX_NAME_LEN];

    printf("Timetable (%d periods). CLASS periods are never used for tasks.\n", manager->periodCount);
    if (manager->periodCount == 0) {
        printf("  (timetable is empty)\n");
        return;
    }

    // First the full timetable, one period per line.
    printf("  %-13s %-6s %-42s %s\n", "Time", "Type", "Period", "Location");
    printf("  ---------------------------------------------------------------------------------\n");
    for (int index = 0; index < manager->periodCount; index++) {
        const Period* period = &manager->periods[index];
        pu_format_time(period->startMinute, startText, sizeof(startText));
        pu_format_time(period->endMinute, endText, sizeof(endText));
        if (period->type == PERIOD_FREE) {
            printf("  %s-%s   %-6s %-42s %s\n", startText, endText, "FREE", period->name, "-");
        } else {
            printf("  %s-%s   %-6s %-42s %s\n", startText, endText, "CLASS", period->name,
                   location_label(locations, period->locationId, place, MAX_NAME_LEN));
        }
    }

    // Then the free slots, with where the student starts and where they must go.
    printf("\nFree slots (%d, total %d minutes)\n", manager->slotCount, sm_total_free_minutes(manager));
    if (manager->slotCount == 0) {
        printf("  (no free periods)\n");
        return;
    }
    for (int index = 0; index < manager->slotCount; index++) {
        const TimeSlot* slot = &manager->freeSlots[index];
        char fromPlace[MAX_NAME_LEN];
        char toPlace[MAX_NAME_LEN];
        pu_format_time(slot->startMinute, startText, sizeof(startText));
        pu_format_time(slot->endMinute, endText, sizeof(endText));
        printf("  Slot %d: %s-%s (%3d min)  start at: %s | must reach: %s\n",
               slot->slotId, startText, endText, slot->durationMinutes,
               location_label(locations, slot->fromLocationId, fromPlace, MAX_NAME_LEN),
               slot->toLocationId == NO_LOCATION ? "(nothing - next period is free or day ends)"
                   : location_label(locations, slot->toLocationId, toPlace, MAX_NAME_LEN));
    }
}

int sm_load_from_file(ScheduleManager* manager, const char* path, int* errorCount)
{
    *errorCount = 0;
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("  [ERROR] Cannot open schedule file: %s\n", path);
        return -1;
    }

    char line[LINE_BUFFER_SIZE];
    int lineNumber = 0;
    int loaded = 0;
    int tooLong = 0;

    // Read one line at a time. A bad line is reported and skipped; the
    // rest of the file is still read. The lines may be in any order,
    // because sm_add_period() sorts them.
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

        char* fields[SCHEDULE_FIELD_COUNT];
        int fieldCount = pu_split(line, FIELD_SEPARATOR, fields, SCHEDULE_FIELD_COUNT);
        if (fieldCount != SCHEDULE_FIELD_COUNT) {
            printf("  [WARN] %s line %d: expected 6 fields, found %d, skipped\n",
                   path, lineNumber, fieldCount);
            (*errorCount)++;
            continue;
        }

        Period period;
        memset(&period, 0, sizeof(period));   // start clean
        if (!pu_parse_int(fields[0], &period.id)) {
            printf("  [WARN] %s line %d: period ID must be a number, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }
        // pu_parse_time accepts "09:30" and also "09:30 AM" / "01:50 PM".
        if (!pu_parse_time(fields[1], &period.startMinute) ||
            !pu_parse_time(fields[2], &period.endMinute)) {
            printf("  [WARN] %s line %d: times must look like 09:30, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }
        if (!pu_copy_text(period.name, MAX_NAME_LEN, fields[3])) {
            printf("  [WARN] %s line %d: period name too long, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }

        // The type decides what the last field must be:
        // a CLASS needs a building number, a FREE period needs "-".
        if (pu_equals_ignore_case(fields[4], "CLASS")) {
            period.type = PERIOD_CLASS;
            if (!pu_parse_int(fields[5], &period.locationId)) {
                printf("  [WARN] %s line %d: a CLASS needs a location ID, skipped\n", path, lineNumber);
                (*errorCount)++;
                continue;
            }
        } else if (pu_equals_ignore_case(fields[4], "FREE")) {
            period.type = PERIOD_FREE;
            period.locationId = NO_LOCATION;
            if (strcmp(fields[5], "-") != 0) {
                printf("  [WARN] %s line %d: a FREE period must use '-' as location, skipped\n",
                       path, lineNumber);
                (*errorCount)++;
                continue;
            }
        } else {
            printf("  [WARN] %s line %d: type must be CLASS or FREE, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }

        // The line can be read - now check it against the rest of the timetable.
        ScheduleStatus status = sm_add_period(manager, &period);
        if (status != SM_OK) {
            printf("  [WARN] %s line %d: period %d rejected (%s)\n",
                   path, lineNumber, period.id, sm_status_message(status));
            (*errorCount)++;
            continue;
        }
        loaded++;
    }

    fclose(file);
    return loaded;
}

const char* sm_status_message(ScheduleStatus status)
{
    switch (status) {
    case SM_OK:
        return "OK";
    case SM_ERR_INVALID_TIME:
        return "start time must be before end time";
    case SM_ERR_OVERLAP:
        return "overlaps another period";
    case SM_ERR_DUPLICATE_ID:
        return "duplicate period ID";
    case SM_ERR_INVALID:
        return "invalid ID or name, or wrong location for the period type";
    case SM_ERR_FULL:
        return "too many periods";
    }
    return "unknown status";
}
