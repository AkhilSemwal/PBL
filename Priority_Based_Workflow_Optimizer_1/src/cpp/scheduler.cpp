#include "scheduler.h"
#include "dijkstra.h"

//stores one plan.
ScheduleResult::ScheduleResult()
{
    clear("(none)");
}

void ScheduleResult::clear(const char* name)
{
    algorithmName = name;
    plannedCount = 0;
    deferredCount = 0;
    slotCount = 0;
    freeMinutes = 0;
    achievedPriority = 0;
    possiblePriority = 0;
}

bool ScheduleResult::addPlanned(const PlannedTask& item, int priority)
{
    if (plannedCount >= MAX_TASKS) {
        return false;
    }
    planned[plannedCount] = item;
    plannedCount++;
    achievedPriority += priority;
    return true;
}

bool ScheduleResult::addDeferred(int taskId, const char* reason)
{
    if (deferredCount >= MAX_TASKS) {
        return false;
    }
    deferred[deferredCount].taskId = taskId;
    deferred[deferredCount].reason = reason;
    deferredCount++;
    return true;
}

bool ScheduleResult::addSlot(const SlotUsage& usage)
{
    if (slotCount >= MAX_SLOTS) {
        return false;
    }
    slots[slotCount] = usage;
    slotCount++;
    return true;
}

void ScheduleResult::setTotals(int free, int possible)
{
    freeMinutes = free;
    possiblePriority = possible;
}

const char* ScheduleResult::getAlgorithmName() const { return algorithmName; }
int ScheduleResult::getPlannedCount() const { return plannedCount; }
const PlannedTask& ScheduleResult::getPlanned(int index) const { return planned[index]; }
int ScheduleResult::getDeferredCount() const { return deferredCount; }
const DeferredTask& ScheduleResult::getDeferred(int index) const { return deferred[index]; }
int ScheduleResult::getSlotCount() const { return slotCount; }
const SlotUsage& ScheduleResult::getSlot(int index) const { return slots[index]; }
int ScheduleResult::getFreeMinutes() const { return freeMinutes; }
int ScheduleResult::getAchievedPriority() const { return achievedPriority; }
int ScheduleResult::getPossiblePriority() const { return possiblePriority; }

bool ScheduleResult::isPlanned(int taskId) const
{
    for (int index = 0; index < plannedCount; index++) {
        if (planned[index].taskId == taskId) {
            return true;
        }
    }
    return false;
}

int ScheduleResult::getTaskMinutes() const
{
    int total = 0;
    for (int index = 0; index < plannedCount; index++) {
        total += planned[index].endMinute - planned[index].startMinute;
    }
    return total;
}

int ScheduleResult::getWalkingMinutes() const
{
    int total = 0;
    for (int index = 0; index < slotCount; index++) {
        total += slots[index].walkingMinutes;
    }
    return total;
}

double ScheduleResult::getTimeUsePercent() const
{
    if (freeMinutes <= 0) {
        return 0.0;
    }
    return 100.0 * getTaskMinutes() / freeMinutes;
}

double ScheduleResult::getPriorityPercent() const
{
    if (possiblePriority <= 0) {
        return 0.0;
    }
    return 100.0 * achievedPriority / possiblePriority;
}

//fixed reasons for tasks that could not be planned.
static const char* const REASON_PREREQUISITE = "waits for a prerequisite that is not done or planned";
static const char* const REASON_UNREACHABLE  = "its location cannot be reached on foot";
static const char* const REASON_TOO_LONG     = "longer than every free slot";
static const char* const REASON_NO_ROOM      = "not enough free time left";

Scheduler::~Scheduler()
{
}

bool Scheduler::plan(const SchedulingInput& input, ScheduleResult& result) const
{
    result.clear(name());

    int freeMinutes = 0;
    int longestSlot = 0;
    for (int index = 0; index < input.slotCount; index++) {
        freeMinutes += input.slots[index].durationMinutes;
        if (input.slots[index].durationMinutes > longestSlot) {
            longestSlot = input.slots[index].durationMinutes;
        }
    }
    int possiblePriority = 0;
    for (int index = 0; index < input.taskCount; index++) {
        if (!input.tasks[index].isCompleted) {
            possiblePriority += input.tasks[index].priority;
        }
    }
    result.setTotals(freeMinutes, possiblePriority);

    //checks if tasks have a dependency cycle.
    int cycle[MAX_TASKS + 1];
    int cycleLength = 0;
    if (tg_find_cycle(input.graph, cycle, &cycleLength) != 0) {
        return false;
    }

    //runs the selected planning algorithm.
    if (!planSlots(input, result)) {
        return false;
    }

    //gives a reason for every task that could not be planned.
    for (int index = 0; index < input.taskCount; index++) {
        const TaskData& task = input.tasks[index];
        if (!isPending(task, result)) {
            continue;
        }

        bool unreachable = false;
        for (int slot = 0; slot < result.getSlotCount(); slot++) {
            int from = result.getSlot(slot).startLocationId;
            if (from != NO_LOCATION && walkMinutes(input.map, from, task.locationId) < 0) {
                unreachable = true;
                break;
            }
        }

        if (!prerequisitesReady(input, task.id, result)) {
            result.addDeferred(task.id, REASON_PREREQUISITE);
        } else if (unreachable) {
            result.addDeferred(task.id, REASON_UNREACHABLE);
        } else if (task.durationMinutes > longestSlot) {
            result.addDeferred(task.id, REASON_TOO_LONG);
        } else {
            result.addDeferred(task.id, REASON_NO_ROOM);
        }
    }
    return true;
}

int Scheduler::walkMinutes(const LocationGraph* map, int fromId, int toId)
{
    if (fromId == NO_LOCATION || toId == NO_LOCATION || fromId == toId) {
        return 0;
    }
    DijkstraSolver solver(map);
    int minutes = 0;
    if (!solver.shortestTime(fromId, toId, minutes)) {
        return -1;   //no walking path between the buildings
    }
    return minutes;
}

bool Scheduler::isPending(const TaskData& task, const ScheduleResult& result)
{
    return !task.isCompleted && !result.isPlanned(task.id);
}

bool Scheduler::prerequisitesReady(const SchedulingInput& input, int taskId, const ScheduleResult& result)
{
    //gets completed tasks and tasks already planned.
    int finished[MAX_TASKS * 2];
    int finishedCount = 0;
    for (int index = 0; index < input.taskCount; index++) {
        if (input.tasks[index].isCompleted) {
            finished[finishedCount] = input.tasks[index].id;
            finishedCount++;
        }
    }
    for (int index = 0; index < result.getPlannedCount(); index++) {
        finished[finishedCount] = result.getPlanned(index).taskId;
        finishedCount++;
    }
    return tg_prerequisites_done(input.graph, taskId, finished, finishedCount) == 1;
}

int Scheduler::slotStartLocation(const TimeSlot& slot, int lastLocationId)
{
    if (slot.fromLocationId != NO_LOCATION) {
        return slot.fromLocationId;   //uses the location of the previous class
    }
    return lastLocationId;            //uses the last slot's ending location
}
