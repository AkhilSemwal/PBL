#include "optimizer.h"

const char* GreedyScheduler::name() const{
    return "Greedy";
}
bool GreedyScheduler::planSlots(const SchedulingInput& input, ScheduleResult& result) const{
    int lastLocation = NO_LOCATION;

    //process every free slot in time order.
    for (int slotIndex = 0; slotIndex < input.slotCount; slotIndex++) {
        const TimeSlot& slot = input.slots[slotIndex];
        int current = slotStartLocation(slot, lastLocation);

        SlotUsage usage;
        usage.slotId = slot.slotId;
        usage.capacityMinutes = slot.durationMinutes;
        usage.startLocationId = current;
        usage.taskMinutes = 0;
        usage.walkingMinutes = 0;
        usage.walkToClassMinutes = 0;
        usage.taskCount = 0;

        int used = 0;   //minutes of this slot already planned

        //fill the slot: pick the best task that fits, again and again.
        while (true) {
            int bestIndex = -1;
            int bestWalk = 0;

            for (int index = 0; index < input.taskCount; index++) {
                const TaskData& task = input.tasks[index];
                if (!isPending(task, result) || !prerequisitesReady(input, task.id, result)){
                    continue;
                }

                int walkIn = walkMinutes(input.map, current, task.locationId);
                int walkOut = walkMinutes(input.map, task.locationId, slot.toLocationId);
                if (walkIn < 0 || walkOut < 0) {
                    continue;   // the building cannot be reached
                }
                if (used + walkIn + task.durationMinutes + walkOut > slot.durationMinutes){
                    continue;   // does not fit in what is left of the slot
                }

                // Task::operator> is the ranking rule: priority, then shorter, then lower ID.
                if (bestIndex == -1 || Task(task) > Task(input.tasks[bestIndex])){
                    bestIndex = index;
                    bestWalk = walkIn;
                }
            }

            if (bestIndex == -1){
                break;   // nothing else fits in this slot
            }

            const TaskData& best = input.tasks[bestIndex];
            PlannedTask item;
            item.taskId = best.id;
            item.slotId = slot.slotId;
            item.walkFromLocationId = (bestWalk > 0) ? current : NO_LOCATION;
            item.walkMinutes = bestWalk;
            item.startMinute = slot.startMinute + used + bestWalk;
            item.endMinute = item.startMinute + best.durationMinutes;
            result.addPlanned(item, best.priority);

            used += bestWalk + best.durationMinutes;
            usage.taskMinutes += best.durationMinutes;
            usage.walkingMinutes += bestWalk;
            usage.taskCount++;
            current = best.locationId;   //the student is now at this building
        }

        //if a class comes next, the walk there is part of this slot.
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

}
