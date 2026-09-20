#include "optimizer.h"
#include <iostream>

using namespace std;


WorkflowOptimizer::WorkflowOptimizer(){}     //constructor

//find highest priority task
Task* WorkflowOptimizer::findHighestPriorityTask(Task** possibleTasks, int count){
    if (count <= 0)
    {
        return nullptr;
    }
    int highestIndex = 0;
    for (int i = 1; i < count; i++)
    {
        if (possibleTasks[i]->priority >
            possibleTasks[highestIndex]->priority)
        {
            highestIndex = i;
        }
    }
    return possibleTasks[highestIndex];
}

optimizeresult WorkflowOptimizer::optimize_workflow( taskmanager& task_mgr, const taskgraph& graph, schedulemanager& sched_mgr ){         // optimize workflow
    optimizeresult result;
    
    sched_mgr.clearAssignments();         // clear old assignment
    
    //get total tasks
    int taskCount = task_mgr.getTaskCount();
    result.total_tasks = taskCount;

    // calculate the total priority
    for (int i = 0; i < taskCount; i++)
    {
        Task* task = task_mgr.getTaskByIndex(i);
        if (task != nullptr)
        {
            result.total_priority += task->priority;
        }
    }
    
    // get the total available time
    result.ava_minutes = sched_mgr.getTotalCapacityAllSlots();
    
    //count completed and pending tasks
    int completedCount = 0;
    int pendingCount = 0;
    for (int i = 0; i < taskCount; i++){
        Task* task = task_mgr.getTaskByIndex(i);
        if (task == nullptr){
            continue;
        }
        if (task->isCompleted){
            completedCount++;
        }
        else{
            pendingCount++;
        }
    }
    
    //dynamic arrays
    int* completed_id = nullptr;
    int* pending_id = nullptr;
    if (completedCount > 0){
        completed_id = new int[completedCount];
    }
    if (pendingCount > 0){
        pending_id = new int[pendingCount];
    }
    
    //fill completed and pending arrays
    int completedIndex = 0;
    int pendingIndex = 0;
    for (int i = 0; i < taskCount; i++){
        Task* task = task_mgr.getTaskByIndex(i);
        if (task == nullptr){
            continue;
        }
        if (task->isCompleted){
            completed_id[completedIndex] = task->id;
            completedIndex++;
        }
        else{
            pending_id[pendingIndex] = task->id;
            pendingIndex++;
        }
    }
    
    //process every time slot
    int slotCount = sched_mgr.getSlotCount();
    for (int s = 0; s < slotCount; s++){
        TimeSlot* slot =
            sched_mgr.getSlotByIndex(s);
        if (slot == nullptr){
            continue;
        }
        bool progress = true;

        //fill current slot
        while (progress && slot->getRemainingMinutes() > 0){
            progress = false;
            
            // Count possible tasks
            int possibleCount = 0;
            for (int i = 0; i < pendingCount; i++){
                Task* task = task_mgr.getTask(pending_id[i]);
                if (task == nullptr){
                    continue;
                }
                bool prerequisiteDone = graph.arePrerequisitesFinished( task->id, completed_id, completedCount);
                bool enoughTime = task->durationMinutes <= slot->getRemainingMinutes();
                if (prerequisiteDone && enoughTime){
                    possibleCount++;
                }
            }
            
            //no suitable task
            if (possibleCount == 0){
                break;
            }
            
            //create dynamic array of possible tasks
            Task** possibleTasks = new Task*[possibleCount];
            int possibleIndex = 0;
            
            //store possible tasks
            for (int i = 0; i < pendingCount; i++){
                Task* task = task_mgr.getTask(pending_id[i]);
                if (task == nullptr){
                    continue;
                }
                bool prerequisiteDone = graph.arePrerequisitesFinished(task->id, completed_id, completedCount);
                bool enoughTime = task->durationMinutes <= slot->getRemainingMinutes();
                if (prerequisiteDone && enoughTime){
                    possibleTasks[possibleIndex] = task;
                    possibleIndex++;
                }
            }
            
            //find highest priority task
            Task* bestTask =findHighestPriorityTask(possibleTasks,possibleCount);
            delete[] possibleTasks;
            if (bestTask == nullptr){
                break;
            }
            
            //assign task
            sched_mgr.assignTaskToSlot(slot->slotId,*bestTask);
            bestTask->scheduledSlotId = slot->slotId;
            
            //add to scheduled list
            result.addScheduledTaskId(bestTask->id);
            
            //add to completed list
            int* newCompleted = new int[completedCount + 1];
            for (int i = 0; i < completedCount; i++){
                newCompleted[i] = completed_id[i];
            }
            newCompleted[completedCount] = bestTask->id;
            delete[] completed_id;
            completed_id = newCompleted;
            completedCount++;
            
            //remove task from pending list
            int newPendingCount = 0;
            for (int i = 0; i < pendingCount; i++){
                if (pending_id[i] != bestTask->id){
                    newPendingCount++;
                }
            }
            int* newPending = nullptr;
            if (newPendingCount > 0){
                newPending = new int[newPendingCount];
                int index = 0;
                for (int i = 0; i < pendingCount; i++){
                    if (pending_id[i] != bestTask->id){
                        newPending[index] = pending_id[i];
                        index++;
                    }
                }
            }
            delete[] pending_id;
            pending_id = newPending;
            pendingCount = newPendingCount;
            progress = true;
        }
    }
    
    //store deferred tasks
    for (int i = 0; i < pendingCount; i++){
        result.addDeferredTaskId(pending_id[i]);
    }
    
    //task counts
    result.schedule_tasks = result.getScheduledCount();
    result.deferred_tasks = result.getDeferredCount();
    
    //used time
    result.used_minutes = sched_mgr.getTotalUsedMinutesAllSlots();
    
    //achieved priority
    for (int i = 0; i < result.getScheduledCount(); i++){
        int id = result.getScheduledId(i);
        Task* task = task_mgr.getTask(id);
        if (task != nullptr){
            result.achieved_priority += task->priority;
        }
    }
    
    //time utilization
    if (result.ava_minutes > 0){
        result.time_utilization = ((double)result.used_minutes / result.ava_minutes) * 100.0;
    }
    
    //priority percentage
    if (result.total_priority > 0){
        result.priority_percent = ((double)result.achieved_priority / result.total_priority) * 100.0;
    }
    
    //free up the memory
    delete[] completed_id;
    delete[] pending_id;
    return result;
}

//print the report
void WorkflowOptimizer::print_report(const optimizeresult& result, const taskmanager& task_mgr) const {     //print optimization report
    cout << "\n";
    cout << "============================================\n";
    cout << "       WORKFLOW OPTIMIZATION REPORT\n";
    cout << "============================================\n";
    cout << "\nTask Information:\n";
    cout << "Total Tasks       : " << result.total_tasks << "\n";
    cout << "Scheduled Tasks   : " << result.schedule_tasks << "\n";
    cout << "Deferred Tasks    : " << result.deferred_tasks << "\n";
    cout << "\nTime Information:\n";
    cout << "Available Time    : " << result.ava_minutes << " minutes\n";
    cout << "Used Time         : " << result.used_minutes << " minutes\n";
    cout << "Unused Time       : " << result.ava_minutes - result.used_minutes << " minutes\n";
    
    cout << "Time Utilization  : " << result.time_utilization << "%\n";
    cout << "\nPriority Information:\n";
    cout << "Total Priority    : " << result.total_priority << "\n";
    cout << "Achieved Priority : " << result.achieved_priority << "\n";
    cout << "Priority Percent  : " << result.priority_percent << "%\n";
    
    //deferred tasks
    if (result.getDeferredCount() > 0){
        cout << "\nDeferred / Postpone Tasks:\n";
        for (int i = 0; i < result.getDeferredCount(); i++){
            int id = result.getDeferredId(i);
            const Task* task = task_mgr.getTask(id);
            if (task != nullptr){
                cout << "ID: " << task->id << " | Name: " << task->name << " | Priority: " << task->priority << " | Duration: " << task->durationMinutes << " min\n";
            }
        }
    }
    cout << "\n============================================\n";
}
