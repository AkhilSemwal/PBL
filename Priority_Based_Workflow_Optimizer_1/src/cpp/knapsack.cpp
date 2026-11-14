/*
 * knapsack.cpp
 * Owner: Piyush (0/1 Knapsack DP planner)
 *
 * See knapsack.h for the idea. Steps for every free slot:
 *   1. collect the ready tasks with their weights and values
 *   2. DP table  best[i][w] = best value using the first i tasks
 *                             in at most w minutes
 *   3. walk back through the table to see which tasks were taken
 *   4. put them in walking order and time the real route (with walking);
 *      if it is too long, shrink the capacity and go back to step 2
 */
#include "knapsack.h"

#include <new>

const char* KnapsackScheduler::name() const
{
    return "0/1 Knapsack";
}

/*
 * Steps 2 and 3: classic 0/1 knapsack.
 * Writes the positions (0 .. itemCount-1) of the chosen items into chosen[]
 * and returns how many were chosen, or -1 if memory ran out.
 */
static int solve_knapsack(const int* weight, const int* value, int itemCount, int capacity,
                          int* chosen)
{
    if (itemCount == 0 || capacity <= 0) {
        return 0;
    }
    int columns = capacity + 1;
    int* best = new (std::nothrow) int[(itemCount + 1) * columns];   // (n + 1) x (W + 1)
    if (best == nullptr) {
        return -1;
    }

    for (int w = 0; w < columns; w++) {
        best[w] = 0;   // row 0: no tasks, value 0
    }
    for (int i = 1; i <= itemCount; i++) {
        for (int w = 0; w < columns; w++) {
            int skip = best[(i - 1) * columns + w];                        // leave task i out
            int take = -1;
            if (weight[i - 1] <= w) {                                      // or take it
                take = best[(i - 1) * columns + (w - weight[i - 1])] + value[i - 1];
            }
            best[i * columns + w] = (take > skip) ? take : skip;
        }
    }

    // Walk back: if the value changed from row i-1 to row i, task i was taken.
    int count = 0;
    int w = capacity;
    for (int i = itemCount; i >= 1; i--) {
        if (best[i * columns + w] != best[(i - 1) * columns + w]) {
            chosen[count] = i - 1;
            count++;
            w -= weight[i - 1];
        }
    }
    delete[] best;
    return count;
}

/*
 * Step 4: orders the chosen tasks (nearest building next, ties by the Task
 * ranking rule) into route[] and returns the real minutes needed, including
 * the walk to the next class. Returns -1 if some building cannot be reached.
 */
static int time_route(const SchedulingInput& input, const TimeSlot& slot, int startLocation,
                      const int* taskIndexes, int count, int* route,
                      int (*walk)(const LocationGraph*, int, int))
{
    bool used[MAX_TASKS] = {false};
    int current = startLocation;
    int minutes = 0;

    for (int step = 0; step < count; step++) {
        int pick = -1;
        int pickWalk = 0;
        for (int position = 0; position < count; position++) {
            if (used[position]) {
                continue;
            }
            const TaskData& task = input.tasks[taskIndexes[position]];
            int distance = walk(input.map, current, task.locationId);
            if (distance < 0) {
                continue;
            }
            if (pick == -1 || distance < pickWalk ||
                (distance == pickWalk && Task(task) > Task(input.tasks[taskIndexes[pick]]))) {
                pick = position;
                pickWalk = distance;
            }
        }
        if (pick == -1) {
            return -1;
        }
        used[pick] = true;
        route[step] = taskIndexes[pick];
        minutes += pickWalk + input.tasks[taskIndexes[pick]].durationMinutes;
        current = input.tasks[taskIndexes[pick]].locationId;
    }

    int toClass = walk(input.map, current, slot.toLocationId);
    if (toClass < 0) {
        return -1;
    }
    return minutes + toClass;
}

bool KnapsackScheduler::planSlots(const SchedulingInput& input, ScheduleResult& result) const
{
    int lastLocation = NO_LOCATION;

    for (int slotIndex = 0; slotIndex < input.slotCount; slotIndex++) {
        const TimeSlot& slot = input.slots[slotIndex];
        int start = slotStartLocation(slot, lastLocation);

        // If we do not know where the student is, estimate from the next class.
        int hub = (start != NO_LOCATION) ? start : slot.toLocationId;

        // ---- Step 1: the items of this knapsack ----
        int items[MAX_TASKS];    // index into input.tasks
        int weight[MAX_TASKS];
        int value[MAX_TASKS];
        int itemCount = 0;
        for (int index = 0; index < input.taskCount; index++) {
            const TaskData& task = input.tasks[index];
            if (!isPending(task, result) || !prerequisitesReady(input, task.id, result)) {
                continue;
            }
            int walk = walkMinutes(input.map, hub, task.locationId);
            if (walk < 0) {
                continue;   // unreachable building
            }
            if (walk + task.durationMinutes > slot.durationMinutes) {
                continue;   // can never fit in this slot
            }
            items[itemCount] = index;
            weight[itemCount] = task.durationMinutes;   // walking is checked in step 4
            // Priority counts first; "+ duration" only breaks ties, because
            // (slot length + 1) is bigger than any total duration in the slot.
            value[itemCount] = task.priority * (slot.durationMinutes + 1) + task.durationMinutes;
            itemCount++;
        }

        // ---- Steps 2-4: solve, time the real route, shrink if needed ----
        int capacity = slot.durationMinutes;
        int chosen[MAX_TASKS];
        int chosenTasks[MAX_TASKS];
        int route[MAX_TASKS];
        int chosenCount = 0;
        while (true) {
            chosenCount = solve_knapsack(weight, value, itemCount, capacity, chosen);
            if (chosenCount < 0) {
                return false;   // out of memory
            }
            if (chosenCount == 0) {
                break;
            }
            for (int position = 0; position < chosenCount; position++) {
                chosenTasks[position] = items[chosen[position]];
            }
            int realMinutes = time_route(input, slot, start, chosenTasks, chosenCount, route,
                                         walkMinutes);
            if (realMinutes >= 0 && realMinutes <= slot.durationMinutes) {
                break;   // the chosen set really fits
            }
            if (realMinutes < 0) {
                capacity--;   // a building on the way is unreachable: try a smaller set
            } else {
                capacity -= realMinutes - slot.durationMinutes;   // walking made it too long by this much
            }
        }

        // ---- Store the plan for this slot ----
        SlotUsage usage;
        usage.slotId = slot.slotId;
        usage.capacityMinutes = slot.durationMinutes;
        usage.startLocationId = start;
        usage.taskMinutes = 0;
        usage.walkingMinutes = 0;
        usage.walkToClassMinutes = 0;
        usage.taskCount = 0;

        int current = start;
        int used = 0;
        for (int step = 0; step < chosenCount; step++) {
            const TaskData& task = input.tasks[route[step]];
            int walk = walkMinutes(input.map, current, task.locationId);

            PlannedTask item;
            item.taskId = task.id;
            item.slotId = slot.slotId;
            item.walkFromLocationId = (walk > 0) ? current : NO_LOCATION;
            item.walkMinutes = walk;
            item.startMinute = slot.startMinute + used + walk;
            item.endMinute = item.startMinute + task.durationMinutes;
            result.addPlanned(item, task.priority);

            used += walk + task.durationMinutes;
            usage.taskMinutes += task.durationMinutes;
            usage.walkingMinutes += walk;
            usage.taskCount++;
            current = task.locationId;
        }

        int toClass = walkMinutes(input.map, current, slot.toLocationId);
        if (toClass > 0) {
            usage.walkToClassMinutes = toClass;
            usage.walkingMinutes += toClass;
        }
        usage.endLocationId = current;
        result.addSlot(usage);
        lastLocation = current;
    }
    return true;
}
