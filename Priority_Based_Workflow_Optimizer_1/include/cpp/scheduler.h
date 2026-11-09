#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include "task_graph.h"
#include "location_graph.h"

//stores one task placed in a free time slot.
typedef struct PlannedTask {
    int taskId;
    int slotId;
    int walkFromLocationId;   //starting location for walking
    int walkMinutes;          //time needed for walking
    int startMinute;          //task start time
    int endMinute;            //task end time
} PlannedTask;

//stores how a free slot is used.
typedef struct SlotUsage {
    int slotId;
    int capacityMinutes;      //total slot time
    int startLocationId;      //starting location
    int endLocationId;        //ending location
    int taskMinutes;          //total task time
    int walkingMinutes;       //total walking time
    int walkToClassMinutes;   //walking time to next class
    int taskCount;            //number of tasks
} SlotUsage;

//stores a task that could not be planned.
typedef struct DeferredTask {
    int taskId;
    const char* reason;       //reason for skipping the task
} DeferredTask;

//stores all the data needed by a planner.
typedef struct SchedulingInput {
    const TaskData* tasks;    //list of all tasks
    int taskCount;
    const TimeSlot* slots;    //free slots in time order
    int slotCount;
    const TaskGraph* graph;   //task dependencies
    const LocationGraph* map; //campus map
} SchedulingInput;

//stores the final schedule using fixed arrays.
class ScheduleResult {
public:
    ScheduleResult();

    void clear(const char* algorithmName);
    bool addPlanned(const PlannedTask& item, int priority);
    bool addDeferred(int taskId, const char* reason);
    bool addSlot(const SlotUsage& usage);
    void setTotals(int freeMinutes, int possiblePriority);

    const char* getAlgorithmName() const;
    int getPlannedCount() const;
    const PlannedTask& getPlanned(int index) const;
    int getDeferredCount() const;
    const DeferredTask& getDeferred(int index) const;
    int getSlotCount() const;
    const SlotUsage& getSlot(int index) const;
    bool isPlanned(int taskId) const;

    int getFreeMinutes() const;
    int getTaskMinutes() const;
    int getWalkingMinutes() const;
    int getAchievedPriority() const;
    int getPossiblePriority() const;
    double getTimeUsePercent() const;      //percentage of time used
    double getPriorityPercent() const;     //percentage of priority achieved

private:
    const char* algorithmName;
    PlannedTask planned[MAX_TASKS];
    int plannedCount;
    DeferredTask deferred[MAX_TASKS];
    int deferredCount;
    SlotUsage slots[MAX_SLOTS];
    int slotCount;
    int freeMinutes;
    int achievedPriority;
    int possiblePriority;
};

//base class for all scheduling algorithms.
class Scheduler {
public:
    virtual ~Scheduler();

    //returns the planner name.
    virtual const char* name() const = 0;

    //sets the totals, stops if the prerequisites have a cycle,
    //runs this planner's planSlots(), then gives every unplanned task a reason.
    bool plan(const SchedulingInput& input, ScheduleResult& result) const;

protected:
    //each planner creates its own schedule.
    virtual bool planSlots(const SchedulingInput& input, ScheduleResult& result) const = 0;

    //returns walking time between two locations using dijkstra.
    //returns 0 for same or missing locations, -1 if unreachable.
    static int walkMinutes(const LocationGraph* map, int fromId, int toId);

    //returns true if the task is not DONE and not planned yet.
    static bool isPending(const TaskData& task, const ScheduleResult& result);

    //checks if all required tasks are done or planned.
    static bool prerequisitesReady(const SchedulingInput& input, int taskId, const ScheduleResult& result);

    //where the student is when the slot starts: the class just before it,
    //or where the previous free slot ended.
    static int slotStartLocation(const TimeSlot& slot, int lastLocationId);
};

#endif
