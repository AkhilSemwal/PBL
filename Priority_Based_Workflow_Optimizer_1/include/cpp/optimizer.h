#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../member1_graph/graph.h"
#include "../member2_scheduletask/taskmanager.h"
#include "../member2_scheduletask/schedulemanager.h"
using namespace std;

class IntArray{        // custom dynamic integer array
    private:
        int* data;
        int size;
        int capacity;
    public:
        IntArray(){
            size = 0;
            capacity = 10;
            data = new int[capacity];
        }
        
        ~IntArray(){
            delete[] data;
        }
        
        void add(int value){
            if (size == capacity){
                capacity = capacity * 2;
                int* newData = new int[capacity];
                for (int i = 0; i < size; i++)
                {
                    newData[i] = data[i];
                }
                delete[] data;
                data = newData;
            }
            data[size] = value;
            size++;
        }
        
        int getSize() const{
            return size;
        }
        
        int get(int index) const{
            if (index < 0 || index >= size){
                return -1;
            }
            return data[index];
        }
        void clear(){
            size = 0;
        }
};

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
    IntArray schedule_id;
    IntArray deferred_id;
    
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

    void addScheduledTaskId(int id){
        schedule_id.add(id);
    }
    void addDeferredTaskId(int id){
        deferred_id.add(id);
    }
    int getScheduledCount() const{
        return schedule_id.getSize();
    }
    int getDeferredCount() const{
        return deferred_id.getSize();
    }
    int getScheduledId(int index) const{
        return schedule_id.get(index);
    }
    int getDeferredId(int index) const{
        return deferred_id.get(index);
    }
};

class WorkflowOptimizer{         //workflow optimizer
    public:
        WorkflowOptimizer();   //constructor
        optimizeresult optimize_workflow(        //optimize workflow
            TaskManager& task_mgr, const TaskGraph& graph,ScheduleManager& sched_mgr
        );

        void print_report(
            const optimizeresult& result, const TaskManager& task_mgr
        )const;
        private:
            Task* findHighestPriorityTask(
            Task** possibleTasks,
            int count
        );
};
#endif
