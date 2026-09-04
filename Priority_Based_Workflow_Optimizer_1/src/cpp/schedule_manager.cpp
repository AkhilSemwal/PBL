#include "schedule_manager.h"
#include <iostream>
#include <iomanip>

// ============================================================================
// MONDAY SCHEDULE MANAGER IMPLEMENTATION
//
// This file contains the one-day schedule for MONDAY.
// ============================================================================


ScheduleManager::ScheduleManager()
{
    // ============================================================
    // MONDAY TIME SLOTS
    // Each college period has a capacity of 55 minutes.
    // ============================================================

    // OOPS with C++ Lab - PCS 307
    addTimeSlot(1, "OOPS with C++ Lab",
                "08:00", "08:55", 55);

    addTimeSlot(2, "OOPS with C++ Lab",
                "08:55", "09:50", 55);


    // Advanced Probability for AI - TCS 368
    addTimeSlot(3, "Advanced Probability for AI",
                "10:10", "11:05", 55);


    // Free period
    addTimeSlot(4, "Free",
                "11:05", "12:00", 55);


    // Logic Design and Computer Organization Lab - PCS 308
    addTimeSlot(5, "Logic Design & Computer Organization Lab",
                "12:00", "12:55", 55);

    addTimeSlot(6, "Logic Design & Computer Organization Lab",
                "12:55", "01:50", 55);


    // Free periods
    addTimeSlot(7, "Free",
                "02:10", "03:05", 55);

    addTimeSlot(8, "Free",
                "03:05", "04:00", 55);

    addTimeSlot(9, "Free",
                "04:00", "04:55", 55);

    addTimeSlot(10, "Free",
                "04:55", "05:50", 55);
}


// ============================================================================
// ADD TIME SLOT
// ============================================================================

void ScheduleManager::addTimeSlot(
    int slotId,
    const std::string& name,
    const std::string& startTime,
    const std::string& endTime,
    int capacityMinutes)
{
    slots.push_back(
        TimeSlot(
            slotId,
            name,
            startTime,
            endTime,
            capacityMinutes
        )
    );
}


// ============================================================================
// GET ALL SLOTS - CONST VERSION
// ============================================================================

const std::vector<TimeSlot>& ScheduleManager::getAllSlots() const
{
    return slots;
}


// ============================================================================
// GET ALL SLOTS - NON-CONST VERSION
// ============================================================================

std::vector<TimeSlot>& ScheduleManager::getAllSlots()
{
    return slots;
}


// ============================================================================
// FIND A TIME SLOT USING SLOT ID
// ============================================================================

TimeSlot* ScheduleManager::getSlot(int slotId)
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        if (slots[i].slotId == slotId)
        {
            return &slots[i];
        }
    }

    return nullptr;
}


// ============================================================================
// CHECK WHETHER A TASK CAN FIT INTO A SLOT
// ============================================================================

bool ScheduleManager::canFitTask(
    int slotId,
    const Task& task) const
{
    for (const TimeSlot& slot : slots)
    {
        if (slot.slotId == slotId)
        {
            return slot.getRemainingMinutes()
                   >= task.durationMinutes;
        }
    }

    return false;
}


// ============================================================================
// ASSIGN TASK TO SLOT
// ============================================================================

bool ScheduleManager::assignTaskToSlot(
    int slotId,
    Task task)
{
    TimeSlot* slot = getSlot(slotId);

    // Slot does not exist
    if (slot == nullptr)
    {
        return false;
    }


    // Check whether task fits
    if (slot->getRemainingMinutes()
        < task.durationMinutes)
    {
        return false;
    }


    // Assign task
    task.scheduledSlotId = slotId;

    slot->tasks.push_back(task);

    slot->usedMinutes += task.durationMinutes;

    return true;
}


// ============================================================================
// CLEAR ALL TASK ASSIGNMENTS
// ============================================================================

void ScheduleManager::clearAssignments()
{
    for (TimeSlot& slot : slots)
    {
        slot.tasks.clear();

        slot.usedMinutes = 0;
    }
}


// ============================================================================
// CALCULATE TOTAL CAPACITY
// ============================================================================

int ScheduleManager::getTotalCapacityAllSlots() const
{
    int total = 0;

    for (const TimeSlot& slot : slots)
    {
        total += slot.totalCapacityMinutes;
    }

    return total;
}


// ============================================================================
// CALCULATE TOTAL USED MINUTES
// ============================================================================

int ScheduleManager::getTotalUsedMinutesAllSlots() const
{
    int total = 0;

    for (const TimeSlot& slot : slots)
    {
        total += slot.usedMinutes;
    }

    return total;
}


// ============================================================================
// CALCULATE OVERALL TIME UTILIZATION
// ============================================================================

double ScheduleManager::getOverallUtilizationPercentage() const
{
    int capacity = getTotalCapacityAllSlots();

    if (capacity <= 0)
    {
        return 0.0;
    }

    return
        ((double)getTotalUsedMinutesAllSlots()
        / capacity) * 100.0;
}


// ============================================================================
// PRINT MONDAY'S SCHEDULE
// ============================================================================

void ScheduleManager::printScheduleTable() const
{
    std::cout
        << "\n========================================================================================\n";

    std::cout
        << "                              MONDAY DAILY TIMETABLE\n";

    std::cout
        << "========================================================================================\n";


    // Check if schedule is empty
    if (slots.empty())
    {
        std::cout
            << " (No time slots configured.)\n";

        std::cout
            << "========================================================================================\n\n";

        return;
    }


    // ============================================================
    // DISPLAY EACH TIME SLOT
    // ============================================================

    for (const TimeSlot& slot : slots)
    {
        std::cout
            << "\n>>> TIME SLOT ["
            << slot.slotId
            << "]: "
            << slot.name
            << " ("
            << slot.startTime
            << " - "
            << slot.endTime
            << ")\n";


        std::cout
            << "    Capacity: "
            << slot.totalCapacityMinutes
            << " mins | Used: "
            << slot.usedMinutes
            << " mins | Free: "
            << slot.getRemainingMinutes()
            << " mins | Utilization: "
            << std::fixed
            << std::setprecision(1)
            << slot.getUtilizationPercentage()
            << "%\n";


        std::cout
            << "    +-----+---------------------------+----------+----------+----------+\n";


        std::cout
            << "    | ID  | Scheduled Task Name       | Duration | Priority | Deadline |\n";


        std::cout
            << "    +-----+---------------------------+----------+----------+----------+\n";


        // ========================================================
        // IF NO TASK IS ASSIGNED
        // ========================================================

        if (slot.tasks.empty())
        {
            std::cout
                << "    |     (Slot is currently empty / idle)                                 |\n";
        }


        // ========================================================
        // DISPLAY ASSIGNED TASKS
        // ========================================================

        else
        {
            for (const Task& t : slot.tasks)
            {
                std::string durationStr =
                    std::to_string(t.durationMinutes)
                    + " mins";


                std::string prioStr =
                    std::to_string(t.priority)
                    + "/10";


                std::string ddlStr =
                    std::to_string(t.deadlineHour)
                    + ":00";


                std::cout
                    << "    | "
                    << std::left
                    << std::setw(3)
                    << t.id

                    << " | "
                    << std::left
                    << std::setw(25)
                    << (t.name.length() > 25
                        ? t.name.substr(0, 22) + "..."
                        : t.name)

                    << " | "
                    << std::left
                    << std::setw(8)
                    << durationStr

                    << " | "
                    << std::left
                    << std::setw(8)
                    << prioStr

                    << " | "
                    << std::left
                    << std::setw(8)
                    << ddlStr

                    << " |\n";
            }
        }


        std::cout
            << "    +-----+---------------------------+----------+----------+----------+\n";
    }


    // ============================================================
    // SUMMARY
    // ============================================================

    std::cout
        << "\n----------------------------------------------------------------------------------------\n";


    std::cout
        << " SUMMARY: Total Capacity: "
        << getTotalCapacityAllSlots()
        << " mins | Total Planned Work: "
        << getTotalUsedMinutesAllSlots()
        << " mins | Overall Time Utilization: "
        << std::fixed
        << std::setprecision(1)
        << getOverallUtilizationPercentage()
        << "%\n";


    std::cout
        << "========================================================================================\n\n";
}