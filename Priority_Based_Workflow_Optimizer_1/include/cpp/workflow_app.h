#ifndef WORKFLOW_APP_H
#define WORKFLOW_APP_H

#include "task_manager.h"        // <-- point this at your real TaskManager/Task header
#include "task_graph.h"
#include "schedule_manager.h"
#include "workflow_optimizer.h"


class WorkflowApp {
public:
    WorkflowApp();

    // Starts the interactive console menu loop. Call this from main().
    void run();

private:
    // --- Shared state (one instance per module) ---
    TaskManager taskMgr;
    TaskGraph graph;
    ScheduleManager schedMgr;
    WorkflowOptimizer optimizer;

    OptimizationResult lastResult;
    bool hasOptimized;

    // --- Setup ---
    void loadSampleScenario();

    // --- Display helpers ---
    void showHeader() const;
    void showMenu() const;

    // --- Menu action handlers ---
    void handleAddTask();
    void handleAddDependency();
    void handleAddTimeSlot();
    void handleSortTasks();
    void handleRunOptimization();
    void handleViewResults();
    void handleRunMemberVerificationTests();
};

#endif // WORKFLOW_APP_H
