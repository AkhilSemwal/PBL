#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include <vector>

#include "../member1_graph/graph.h"
#include "../member2_scheduletask/taskmanager.h"
#include "../member2_scheduletask/schedulemanager"
using namespace std;

struct optimizeresult{       //store final result of optimization
    int total_tasks;
    int schedule_tasks;
    int deferred_tasks;     //delayed/postpone tasks until further time
    int total_priority;
    int achieved_priority;
    int ava_minutes;
    int used_minutes;
    double time_utilization;
    double priority_percent;
    vector<int> schedule_id;
    vector<int> deferred_id;
    
    optimizeresult(){       //constructor
        total_tasks=0;
        schedule_tasks=0;
        deferred_tasks=0;
        total_priority=0;
        achieved_priority=0;
        ava_minutes=0;
        used_minutes=0;
        time_utilization=0.0;
        priority_percent=0.0;
    }
};

class WorkflowOptimizer{
    public:
        WorkflowOptimizer();   //constructor
        optimizeresult optimize_workflow(        //optimize workflow
            TaskManager& task_mgr, const TaskGraph& graph,ScheduleManager& sched_mgr
        );

        void print_report(
            const optimizeresult& result, const TaskManager& task_mgr
        )const;
};
#endif