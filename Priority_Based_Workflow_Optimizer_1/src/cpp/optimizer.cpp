#include "optimizer.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

using namespace std;


WorkflowOptimizer::WorkflowOptimizer(){}     //constructor

optimizeresult WorkflowOptimizer::optimize_workflow( taskmanager& task_mgr, const taskgraph& graph, schedulemanager& sched_mgr ){         // optimize workflow
    optimizeresult result;
    
    sched_mgr.clearAssignments();         // clear old assignment

    vector<Task>& tasks = task_mgr.getAllTasks();       // get all task
    result.total_tasks = tasks.size();
    for (const Task& task : tasks){               // calculate the total priority
    
        result.total_priority += task.priority;
    }
    
    result.ava_minutes = sched_mgr.getTotalCapacityAllSlots();         // get the total available time
    vector<int> completed_id;                        // store the ids of completed task
    for (const Task& task : tasks){
    
        if (task.isCompleted){
        
            completed_id.push_back(task.id);
        }
    }
    
    vector<int> pending_id;           //store the ids pending task
    for (const Task& task : tasks){
        if (!task.isCompleted){
            pending_id.push_back(task.id);
        }
    }
    
    vector<TimeSlot>& slots = sched_mgr.getAllSlots();         //get all the time slot

    for (TimeSlot& slot : slots){               //check the every time slot
        bool progress = true;
        
        while (progress && slot.getRemainingMinutes() > 0){       //continue while there is free time
            progress = false;
            vector<Task*> possible_tasks;             //store the tasks which can be scheduled

            for (int id : pending_id){                //check every pending task
                Task* task = task_mgr.getTask(id);
                
                if (task == nullptr){          //if the task does not exist
                    continue;
                }

                bool prerequisite_done = graph.arePrerequisitesFinished(id, completed_id);       //check the prerequisites
               
                bool enough_time = task->durationMinutes <= slot.getRemainingMinutes();         //check the available time
                
                if (prerequisite_done && enough_time){                      //task can be scheduled
                    possible_tasks.push_back(task);
                }
            }
            if (!possible_tasks.empty()){                             //check if there are possible tasks
                
                sort(possible_tasks.begin(), possible_tasks.end(), [](const Task* a, const Task* b){          //sort the tasks according to priority
                        return a->priority > b->priority;
                    }
                );

                Task* best_task = possible_tasks[0];         //select the highest priority task
                
                sched_mgr.assignTaskToSlot(slot.slotId, *best_task );         //assign the task to slot
                
                best_task->scheduledSlotId = slot.slotId;          //save slot ID in task
                // Store scheduled task ID
                result.schedule_id.push_back( best_task->id);       //store the scheduled task ID

                completed_id.push_back(best_task->id);             //add task to completed list
        
                pending_id.erase(
                    remove( pending_id.begin(), pending_id.end(), best_task->id ), pending_id.end()       //remove task from pending list
                );
                progress = true;
            }
        }
    }
    
    result.deferred_id = pending_id;             //remaining tasks are deferred/postpone
    
    result.schedule_tasks = result.schedule_id.size();      //count the tasks
    result.deferred_tasks = result.deferred_id.size();
    
    result.used_minutes = sched_mgr.getTotalUsedMinutesAllSlots();           //calculate the used time
    
    for (int id : result.schedule_id){                     //calculate the achieved priority
        const Task* task = task_mgr.getTask(id);
        if (task != nullptr){
            result.achieved_priority += task->priority;
        }
    }
    
    if (result.ava_minutes > 0){                          //calculate time utilization
        result.time_utilization = ((double)result.used_minutes / result.ava_minutes) * 100.0;
    }
    
    if (result.total_priority > 0){               //calculate the priority percentage
        result.priority_percent = ((double)result.achieved_priority / result.total_priority) * 100.0;
    }
    return result;
}

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
    cout << fixed << setprecision(1);
    cout << "Time Utilization  : " << result.time_utilization << "%\n";
    cout << "\nPriority Information:\n";
    cout << "Total Priority    : " << result.total_priority << "\n";
    cout << "Achieved Priority : " << result.achieved_priority << "\n";
    cout << "Priority Percent  : " << result.priority_percent << "%\n";
    
    if (!result.deferred_id.empty()){               //display the deferred/postpone tasks
        cout << "\nDeferred / Postpone Tasks:\n";
        for (int id : result.deferred_id){
            const Task* task = task_mgr.getTask(id);
            if (task != nullptr){
                cout << "ID: " << task->id << " | Name: " << task->name << " | Priority: " << task->priority << " | Duration: " << task->durationMinutes << " min\n";
            }
        }
    }
    cout << "\n============================================\n";
}