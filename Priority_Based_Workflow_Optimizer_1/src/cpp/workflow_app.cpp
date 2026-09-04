#include "workflow_optimizer.h"
#include "task_manager.h"   // <-- assumes TaskManager has: std::vector<Task> getAllTasks() const;
#include <algorithm>
#include <iostream>
#include <set>

OptimizationResult WorkflowOptimizer::optimizeWorkflow(TaskManager& taskMgr, TaskGraph& graph, ScheduleManager& schedMgr) {
    OptimizationResult result;

    std::vector<Task> tasks = taskMgr.getAllTasks();

    // Greedy: highest priority first
    std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.priority > b.priority;
    });

    std::set<int> scheduledIds;
    int totalCapacity = 0;
    int totalUsed = 0;

    for (const auto& slot : schedMgr.getAllSlots()) {
        totalCapacity += slot.capacityMinutes;
    }

    for (const auto& task : tasks) {
        // Skip if any prerequisite hasn't been scheduled yet
        std::vector<int> prereqs = graph.getPrerequisites(task.id);
        bool prereqsDone = true;
        for (int p : prereqs) {
            if (scheduledIds.find(p) == scheduledIds.end()) {
                prereqsDone = false;
                break;
            }
        }
        if (!prereqsDone) continue;

        // Try each slot in order until one has room
        bool placed = false;
        for (const auto& slot : schedMgr.getAllSlots()) {
            if (schedMgr.assignTaskToSlot(slot.id, task)) {
                placed = true;
                break;
            }
        }

        if (placed) {
            scheduledIds.insert(task.id);
            result.tasksScheduledCount++;
            result.totalPriorityPointsAchieved += task.priority;
            totalUsed += task.duration;
        }
    }

    result.timeUtilizationPercent = (totalCapacity > 0)
        ? (100.0 * totalUsed / totalCapacity)
        : 0.0;

    return result;
}

void WorkflowOptimizer::printOptimizationReport(const OptimizationResult& result, TaskManager& taskMgr) const {
    std::cout << "\n--- Optimization Report (Basic Greedy Pass) ---\n";
    std::cout << "  Tasks Scheduled:        " << result.tasksScheduledCount << "\n";
    std::cout << "  Time Utilization:       " << result.timeUtilizationPercent << "%\n";
    std::cout << "  Priority Points Gained: " << result.totalPriorityPointsAchieved << "\n";
    std::cout << "  (Note: this is a basic greedy pass, not a fully optimal schedule.)\n";
}