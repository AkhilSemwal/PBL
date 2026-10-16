/*
 * workflow_app.h
 * Owner: Akhil (integration, console application)
 *
 * WorkflowApp owns every module object, loads the data files and shows the
 * console menu. It only COORDINATES: each algorithm lives in its own module.
 *
 * Main menu (0 = Exit). "Manage" options open a sub-menu (0 = Back).
 *   1 Manage Locations      4 Manage To-Do List     7 View Itinerary  (Phase 3)
 *   2 Manage Routes         5 View Schedule         8 Save Data       (Phase 3)
 *   3 Manage Fixed Schedule 6 Optimize My Day (Phase 3)   9 Load Data
 *
 * Options marked "(Phase 3)" print a short message in this phase.
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
    void showPhase3Message(const char* feature) const;

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

    /* ---- input helpers (all input is read line by line) ---- */
    bool readLine(const char* prompt, char* buffer, int bufferSize);
    bool readInteger(const char* prompt, int minimum, int maximum, int& value);
    bool readLocationId(const char* prompt, int& locationId);
    const char* taskNameOf(int taskId) const;

    /* ---- module objects ---- */
    TaskManager taskManager;
    ScheduleManager scheduleManager;
    LocationGraph locationGraph;
    TaskGraph taskGraph;

    Task* taskObjects;       /* new[]-allocated Task objects (OOP view of the tasks) */
    int taskObjectCount;

    /* Phase 3: GreedyScheduler, KnapsackScheduler and their ScheduleResults. */

    bool inputClosed;        /* true after end-of-input (Ctrl+D / Ctrl+Z) */
};

#endif /* WORKFLOW_APP_H */
