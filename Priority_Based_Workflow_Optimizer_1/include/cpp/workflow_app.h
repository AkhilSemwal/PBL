/*
 * workflow_app.h
 * Owner: Akhil (integration, console application)
 *
 * WorkflowApp owns every module object, loads the data files and shows the
 * console menu. It only COORDINATES: each algorithm lives in its own module.
 *
 * Main menu (0 = Exit). "Manage" options open a sub-menu (0 = Back).
 *   1 Manage Locations      4 Manage To-Do List     7 View Itinerary
 *   2 Manage Routes         5 View Schedule         8 Save Data
 *   3 Manage Fixed Schedule 6 Optimize My Day       9 Load Data
 *
 * Phase 3: the two planners (Greedy, 0/1 Knapsack) are used through a
 * Scheduler pointer, the chosen plan is kept in currentPlan, and every
 * list can be edited from the menu and saved back to data/.
 *
 * Data files are opened with paths relative to the project folder
 * (data/tasks.txt, ...), so the program must be started from that folder.
 */
#ifndef WORKFLOW_APP_H
#define WORKFLOW_APP_H

#include "task.h"
#include "task_manager.h"
#include "schedule_manager.h"
#include "location_graph.h"
#include "task_graph.h"
#include "scheduler.h"
#include "optimizer.h"
#include "knapsack.h"

class WorkflowApp {
public:
    WorkflowApp();
    ~WorkflowApp();

    /* Loads and validates all data files. Returns false if any file had a problem. */
    bool initialize();

    /* Shows the menu until the user chooses Exit (or input ends). */
    void run();

private:
    /* Not copyable: the object owns dynamically allocated memory. */
    WorkflowApp(const WorkflowApp&) = delete;
    WorkflowApp& operator=(const WorkflowApp&) = delete;

    /* ---- data handling ---- */
    bool loadAllData();
    void releaseData();
    bool validateData();
    void rebuildTaskGraph(bool reportProblems);
    bool rebuildTaskObjects();

    /* ---- screen helpers (box menus) ---- */
    void printBoxBorder() const;
    void printMenuBox(const char* title, const char* const items[], int itemCount, const char* zeroLabel) const;
    void printOk(const char* message) const;
    void printLocationList() const;

    /* ---- main menu ---- */
    void printMenu() const;
    bool handleChoice(int choice);

    /* ---- sub-menus ---- */
    void manageLocations();
    void manageRoutes();
    void manageSchedule();
    void manageTodo();
    void optimizeMyDay();

    /* ---- actions ---- */
    void showLocationGraph() const;
    void findShortestRoute();
    void showTimetable() const;
    void showAllTasks() const;
    void showPendingTasks() const;
    void showDependencyGraph() const;
    void checkDependencyCycles() const;
    void showTasksByPriority() const;
    void reloadData();

    /* ---- Phase 3: planning ---- */
    void makeSchedulingInput(SchedulingInput& input) const;
    void runPlanner(const Scheduler& planner);
    void comparePlanners();
    void printPlanSummary(const ScheduleResult& plan) const;
    void showItinerary() const;
    void printWalk(int startMinute, int fromId, int toId, int minutes) const;
    const TimeSlot* findSlot(int slotId) const;

    /* ---- Phase 3: editing and saving ---- */
    void addLocation();
    void addRoute();
    void removeRoute();
    void addPeriod();
    void deletePeriod();
    void addTask();
    void updateTask();
    void markTaskDone();
    void deleteTask();
    void showTaskOrder() const;
    bool readTaskFields(TaskData& task, bool keepAllowed);
    bool applyTaskGraphChange(const TaskData* undoRecord, int addedTaskId);
    void markDataChanged();
    void saveData();

    /* ---- input helpers (all input is read line by line) ---- */
    bool readLine(const char* prompt, char* buffer, int bufferSize);
    bool readInteger(const char* prompt, int minimum, int maximum, int& value);
    bool readLocationId(const char* prompt, int& locationId);
    bool readIntegerOrKeep(const char* prompt, int minimum, int maximum, int& value);
    bool readTime(const char* prompt, int& minutes);
    const char* taskNameOf(int taskId) const;

    /* ---- module objects ---- */
    TaskManager taskManager;
    ScheduleManager scheduleManager;
    LocationGraph locationGraph;
    TaskGraph taskGraph;

    Task* taskObjects;       /* new[]-allocated Task objects (OOP view of the tasks) */
    int taskObjectCount;

    /* ---- Phase 3: planners and the plan the user chose ---- */
    GreedyScheduler greedyPlanner;
    KnapsackScheduler knapsackPlanner;
    ScheduleResult currentPlan;
    bool hasPlan;              /* currentPlan matches the current data      */
    bool hasUnsavedChanges;    /* something was edited since the last save  */

    bool inputClosed;        /* true after end-of-input (Ctrl+D / Ctrl+Z) */
};

#endif /* WORKFLOW_APP_H */
