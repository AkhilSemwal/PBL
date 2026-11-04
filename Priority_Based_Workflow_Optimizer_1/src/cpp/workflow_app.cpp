/*
 * workflow_app.cpp
 * Owner: Akhil (integration, console application)
 *
 * Connects all modules and implements the console menu.
 */
#include "workflow_app.h"
#include "dijkstra.h"
#include "task.h"
#include "scheduler.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>

/* Data file locations, relative to the project root. */
static const char* const TASKS_FILE = "data/tasks.txt";
static const char* const SCHEDULE_FILE = "data/schedule.txt";
static const char* const LOCATIONS_FILE = "data/locations.txt";
static const char* const ROUTES_FILE = "data/routes.txt";

/* Inside width of the menu boxes. */
static const int BOX_WIDTH = 60;

/* Size of the buffer for one line typed by the user. */
static const int INPUT_BUFFER_SIZE = 128;

/* ===================== construction / data ===================== */

WorkflowApp::WorkflowApp()
    : taskObjects(nullptr),
      taskObjectCount(0),
      hasPlan(false),
      hasUnsavedChanges(false),
      inputClosed(false)
{
    taskManager.tasks = nullptr;
    taskManager.count = 0;
    taskManager.capacity = 0;
    sm_init(&scheduleManager);
    lg_init(&locationGraph);
    tg_init(&taskGraph);
}

WorkflowApp::~WorkflowApp()
{
    releaseData();
}

void WorkflowApp::releaseData()
{
    tm_free(&taskManager);
    sm_init(&scheduleManager);
    lg_free(&locationGraph);
    tg_free(&taskGraph);
    delete[] taskObjects;
    taskObjects = nullptr;
    taskObjectCount = 0;
}

bool WorkflowApp::initialize()
{
    return loadAllData();
}

bool WorkflowApp::loadAllData()
{
    bool allGood = true;
    int errors = 0;

    if (!tm_init(&taskManager)) {
        std::cout << "[ERROR] Not enough memory for the task list.\n";
        return false;
    }

    /* Locations first: the other files refer to location IDs.
       Each loader prints a [WARN] / [ERROR] line itself if something is wrong. */
    if (lg_load_locations(&locationGraph, LOCATIONS_FILE, &errors) < 0 || errors > 0) {
        allGood = false;
    }
    if (lg_load_routes(&locationGraph, ROUTES_FILE, &errors) < 0 || errors > 0) {
        allGood = false;
    }
    if (sm_load_from_file(&scheduleManager, SCHEDULE_FILE, &errors) < 0 || errors > 0) {
        allGood = false;
    }
    if (tm_load_from_file(&taskManager, TASKS_FILE, &errors) < 0 || errors > 0) {
        allGood = false;
    }

    rebuildTaskGraph(true);
    if (!rebuildTaskObjects()) {
        allGood = false;
    }
    if (!validateData()) {
        allGood = false;
    }

    if (allGood) {
        printOk("Existing data loaded from data/ directory.");
    } else {
        std::cout << "[!] Some data could not be loaded. The program continues with the data it has.\n"
                  << "    (Run the program from the project folder so that data/ can be found.)\n";
    }
    return allGood;
}

void WorkflowApp::rebuildTaskGraph(bool reportProblems)
{
    tg_free(&taskGraph);
    tg_init(&taskGraph);

    /* One vertex per task. */
    for (int index = 0; index < tm_count(&taskManager); index++) {
        const TaskData* task = tm_get_task_at(&taskManager, index);
        tg_add_task(&taskGraph, task->id, task->priority);
    }

    /* One edge per prerequisite: prerequisite -> task. */
    for (int index = 0; index < tm_count(&taskManager); index++) {
        const TaskData* task = tm_get_task_at(&taskManager, index);
        for (int position = 0; position < task->prerequisiteCount; position++) {
            int prerequisiteId = task->prerequisiteIds[position];
            TaskGraphStatus status = tg_add_prerequisite(&taskGraph, prerequisiteId, task->id);
            if (status != TG_OK && reportProblems) {
                std::cout << "  [WARN] T" << task->id << " needs T" << prerequisiteId
                          << ": " << tg_status_message(status) << "\n";
            }
        }
    }
}

bool WorkflowApp::rebuildTaskObjects()
{
    delete[] taskObjects;
    taskObjects = nullptr;
    taskObjectCount = 0;

    int count = tm_count(&taskManager);
    if (count == 0) {
        return true;
    }
    taskObjects = new (std::nothrow) Task[count];
    if (taskObjects == nullptr) {
        std::cout << "  [ERROR] Not enough memory for task objects.\n";
        return false;
    }
    for (int index = 0; index < count; index++) {
        taskObjects[index] = Task(*tm_get_task_at(&taskManager, index));
    }
    taskObjectCount = count;
    return true;
}

bool WorkflowApp::validateData()
{
    /* Prints a [WARN] line only when something is wrong. Returns true if all is fine. */
    int problems = 0;

    for (int index = 0; index < tm_count(&taskManager); index++) {
        const TaskData* task = tm_get_task_at(&taskManager, index);
        if (lg_find_index_by_id(&locationGraph, task->locationId) == -1) {
            std::cout << "  [WARN] T" << task->id << " uses unknown location " << task->locationId << "\n";
            problems++;
        }
    }
    for (int index = 0; index < sm_period_count(&scheduleManager); index++) {
        const Period* period = sm_get_period(&scheduleManager, index);
        if (period->type == PERIOD_CLASS &&
            lg_find_index_by_id(&locationGraph, period->locationId) == -1) {
            std::cout << "  [WARN] period " << period->id << " uses unknown location "
                      << period->locationId << "\n";
            problems++;
        }
    }

    int cycle[MAX_TASKS + 1];
    int cycleLength = 0;
    if (tg_find_cycle(&taskGraph, cycle, &cycleLength) == 1) {
        std::cout << "  [WARN] prerequisite cycle: ";
        for (int position = 0; position < cycleLength; position++) {
            std::cout << "T" << cycle[position] << (position + 1 < cycleLength ? " -> " : "\n");
        }
        problems++;
    }
    return problems == 0;
}

const char* WorkflowApp::taskNameOf(int taskId) const
{
    const TaskData* task = tm_find_task(&taskManager, taskId);
    if (task == nullptr) {
        return "(unknown task)";
    }
    return task->name;
}

/* ===================== input helpers ===================== */

bool WorkflowApp::readLine(const char* prompt, char* buffer, int bufferSize)
{
    buffer[0] = '\0';
    if (inputClosed) {
        return false;
    }
    std::cout << prompt;
    std::cout.flush();

    std::cin.getline(buffer, bufferSize);
    if (std::cin.eof() && buffer[0] == '\0') {
        inputClosed = true;
        std::cout << "\n";
        return false;
    }
    if (std::cin.fail()) {
        /* Line longer than the buffer: discard the rest and report it as empty. */
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Input too long.\n";
        buffer[0] = '\0';
        return true;
    }
    pu_trim(buffer);
    return true;
}

bool WorkflowApp::readInteger(const char* prompt, int minimum, int maximum, int& value)
{
    char buffer[INPUT_BUFFER_SIZE];
    while (readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
        int number = 0;
        if (pu_parse_int(buffer, &number) && number >= minimum && number <= maximum) {
            value = number;
            return true;
        }
        std::cout << "  Please enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
    return false;
}

bool WorkflowApp::readLocationId(const char* prompt, int& locationId)
{
    char buffer[INPUT_BUFFER_SIZE];
    while (readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
        int number = 0;
        int index = -1;
        if (pu_parse_int(buffer, &number)) {
            index = lg_find_index_by_id(&locationGraph, number);
        } else if (buffer[0] != '\0') {
            index = lg_find_index_by_name(&locationGraph, buffer);
        }
        if (index != -1) {
            locationId = lg_get_id(&locationGraph, index);
            return true;
        }
        std::cout << "  Unknown location. Enter an ID from the list or the exact name.\n";
    }
    return false;
}

/* ===================== screen helpers (box menu) ===================== */

void WorkflowApp::printBoxBorder() const
{
    std::cout << "+";
    for (int column = 0; column < BOX_WIDTH; column++) {
        std::cout << "-";
    }
    std::cout << "+\n";
}

void WorkflowApp::printMenuBox(const char* title, const char* const items[], int itemCount,
                               const char* zeroLabel) const
{
    std::cout << "\n";
    printBoxBorder();

    /* Title, centred. */
    int titleLength = static_cast<int>(std::strlen(title));
    int leftPad = (BOX_WIDTH - titleLength) / 2;
    int rightPad = BOX_WIDTH - titleLength - leftPad;
    std::cout << "|";
    for (int column = 0; column < leftPad; column++) {
        std::cout << " ";
    }
    std::cout << title;
    for (int column = 0; column < rightPad; column++) {
        std::cout << " ";
    }
    std::cout << "|\n";
    printBoxBorder();

    /* Items 1..n, then 0. Text is padded with %-*s so the right border lines up. */
    char line[96];
    for (int index = 0; index <= itemCount; index++) {
        bool isZero = (index == itemCount);
        int number = isZero ? 0 : index + 1;
        const char* text = isZero ? zeroLabel : items[index];
        int numberWidth = (number >= 10) ? 2 : 1;   /* "[10]" is one character wider */
        snprintf(line, sizeof(line), " %-*s", BOX_WIDTH - 5 - numberWidth, text);
        std::cout << "|  [" << number << "]" << line << "|\n";
    }
    printBoxBorder();
}

void WorkflowApp::printOk(const char* message) const
{
    std::cout << "[OK] " << message << "\n";
}

void WorkflowApp::printLocationList() const
{
    for (int index = 0; index < lg_vertex_count(&locationGraph); index++) {
        std::cout << "  [" << lg_get_id(&locationGraph, index) << "] "
                  << lg_get_name(&locationGraph, index) << "\n";
    }
}

/* ===================== main menu ===================== */

void WorkflowApp::printMenu() const
{
    static const char* const items[] = {
        "Manage Locations",
        "Manage Routes",
        "Manage Fixed Schedule",
        "Manage To-Do List",
        "View Schedule",
        "Optimize My Day",
        "View Itinerary",
        "Save Data",
        "Load Data"
    };
    printMenuBox("PRIORITY BASED WORKFLOW OPTIMIZER", items, 9, "Exit");
}

void WorkflowApp::run()
{
    bool keepRunning = true;
    while (keepRunning) {
        printMenu();
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 9, choice)) {
            break;   /* input ended */
        }
        keepRunning = handleChoice(choice);
    }

    /* Do not lose edits by accident. */
    if (hasUnsavedChanges && !inputClosed) {
        int answer = 0;
        if (readInteger("You have unsaved changes. Save them now? (1 = yes, 0 = no): ", 0, 1, answer)
            && answer == 1) {
            saveData();
        }
    }
    std::cout << "Goodbye! Memory is released when the program closes.\n";
}

bool WorkflowApp::handleChoice(int choice)
{
    switch (choice) {
    case 1:
        manageLocations();
        break;
    case 2:
        manageRoutes();
        break;
    case 3:
        manageSchedule();
        break;
    case 4:
        manageTodo();
        break;
    case 5:
        showTimetable();
        break;
    case 6:
        optimizeMyDay();
        break;
    case 7:
        showItinerary();
        break;
    case 8:
        saveData();
        break;
    case 9:
        reloadData();
        break;
    case 0:
        return false;
    default:
        std::cout << "Invalid choice.\n";
        break;
    }
    return !inputClosed;
}

void WorkflowApp::showTimetable() const
{
    sm_display_timetable(&scheduleManager, &locationGraph);
}

void WorkflowApp::showAllTasks() const
{
    tm_display_all(&taskManager, &locationGraph);
}

void WorkflowApp::showPendingTasks() const
{
    tm_display_pending(&taskManager, &locationGraph);
}

void WorkflowApp::showDependencyGraph() const
{
    tg_display(&taskGraph);
}

void WorkflowApp::checkDependencyCycles() const
{
    int cycle[MAX_TASKS + 1];
    int cycleLength = 0;
    int found = tg_find_cycle(&taskGraph, cycle, &cycleLength);

    if (found == -1) {
        std::cout << "Out of memory while checking for cycles.\n";
        return;
    }
    if (found == 0) {
        std::cout << "No dependency cycle found (DFS checked " << tg_vertex_count(&taskGraph)
                  << " tasks). A valid prerequisite order exists.\n";
        return;
    }
    std::cout << "Dependency cycle found:\n  ";
    for (int position = 0; position < cycleLength; position++) {
        std::cout << "T" << cycle[position] << " " << taskNameOf(cycle[position]);
        if (position + 1 < cycleLength) {
            std::cout << "\n   -> ";
        }
    }
    std::cout << "\nTasks in or behind this cycle can never be scheduled until the data is fixed.\n";
}

void WorkflowApp::showLocationGraph() const
{
    showGraph(&locationGraph);
}

void WorkflowApp::findShortestRoute()
{
    if (lg_vertex_count(&locationGraph) == 0) {
        std::cout << "No locations loaded.\n";
        return;
    }

    std::cout << "Locations:\n";
    printLocationList();

    int sourceId = 0;
    int destinationId = 0;
    if (!readLocationId("From (ID or name): ", sourceId)) {
        return;
    }
    if (!readLocationId("To   (ID or name): ", destinationId)) {
        return;
    }

    DijkstraSolver solver(&locationGraph);
    ShortestPath path;
    DijkstraStatus status = solver.findPath(sourceId, destinationId, path);
    if (status != DIJKSTRA_OK) {
        std::cout << "No route: " << DijkstraSolver::statusMessage(status) << ".\n";
        return;
    }
    std::cout << "Shortest route: ";
    solver.printPath(path);
}

void WorkflowApp::reloadData()
{
    releaseData();
    sm_init(&scheduleManager);
    lg_init(&locationGraph);
    tg_init(&taskGraph);
    loadAllData();
    hasPlan = false;             /* the old plan belongs to the old data */
    hasUnsavedChanges = false;
}

/* ===================== sub-menus ===================== */

void WorkflowApp::manageTodo()
{
    static const char* const items[] = {
        "View all tasks",
        "View pending tasks",
        "View pending tasks by priority",
        "Show dependency graph",
        "Check for dependency cycles (DFS)",
        "Show valid task order (Kahn's algorithm)",
        "Add a task",
        "Update a task",
        "Mark a task as done",
        "Delete a task"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE TO-DO LIST", items, 10, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 10, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showAllTasks(); break;
        case 2: showPendingTasks(); break;
        case 3: showTasksByPriority(); break;
        case 4: showDependencyGraph(); break;
        case 5: checkDependencyCycles(); break;
        case 6: showTaskOrder(); break;
        case 7: addTask(); break;
        case 8: updateTask(); break;
        case 9: markTaskDone(); break;
        default: deleteTask(); break;
        }
    }
}

/*
 * Lists pending tasks from most to least important.
 * Uses the Task class: operator> decides the order (higher priority, then
 * shorter, then lower ID) and operator<< prints each task.
 * Sorting: insertion sort on an array of indices (no std::sort).
 */
void WorkflowApp::showTasksByPriority() const
{
    if (taskObjectCount == 0) {
        std::cout << "No tasks loaded.\n";
        return;
    }
    int* order = new (std::nothrow) int[taskObjectCount];
    if (order == nullptr) {
        std::cout << "Out of memory.\n";
        return;
    }

    /* Collect the pending tasks. */
    int count = 0;
    for (int index = 0; index < taskObjectCount; index++) {
        if (!taskObjects[index].isCompleted()) {
            order[count] = index;
            count++;
        }
    }

    /* Insertion sort: move each task left while it ranks higher than its neighbour. */
    for (int position = 1; position < count; position++) {
        int current = order[position];
        int slot = position;
        while (slot > 0 && taskObjects[current] > taskObjects[order[slot - 1]]) {
            order[slot] = order[slot - 1];
            slot--;
        }
        order[slot] = current;
    }

    std::cout << "Pending tasks, most important first (rule: priority, then shorter, then lower ID)\n";
    for (int rank = 0; rank < count; rank++) {
        const Task& task = taskObjects[order[rank]];
        std::cout << "  " << (rank + 1 < 10 ? " " : "") << rank + 1 << ". " << task
                  << " @ " << lg_name_of(&locationGraph, task.getLocationId()) << "\n";
    }
    std::cout << "The Greedy planner uses this same rule to pick the next task for a free slot.\n";
    delete[] order;
}

void WorkflowApp::manageLocations()
{
    static const char* const items[] = {
        "View all locations (campus map)",
        "Find shortest walking route (Dijkstra)",
        "Add a location"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE LOCATIONS", items, 3, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 3, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showLocationGraph(); break;
        case 2: findShortestRoute(); break;
        default: addLocation(); break;
        }
    }
}

void WorkflowApp::manageRoutes()
{
    static const char* const items[] = {
        "View all routes (campus map)",
        "Find shortest walking route (Dijkstra)",
        "Add a route",
        "Remove a route"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE ROUTES", items, 4, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 4, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showLocationGraph(); break;
        case 2: findShortestRoute(); break;
        case 3: addRoute(); break;
        default: removeRoute(); break;
        }
    }
}

void WorkflowApp::manageSchedule()
{
    static const char* const items[] = {
        "View timetable and free slots",
        "Add a period",
        "Delete a period"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE FIXED SCHEDULE", items, 3, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 3, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showTimetable(); break;
        case 2: addPeriod(); break;
        default: deletePeriod(); break;
        }
    }
}

void WorkflowApp::optimizeMyDay()
{
    static const char* const items[] = {
        "Plan with Greedy",
        "Plan with 0/1 Knapsack DP",
        "Compare Greedy and Knapsack"
    };
    while (!inputClosed) {
        printMenuBox("OPTIMIZE MY DAY", items, 3, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 3, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: runPlanner(greedyPlanner); break;
        case 2: runPlanner(knapsackPlanner); break;
        default: comparePlanners(); break;
        }
    }
}

/* ===================== Phase 3: input helpers ===================== */

/* Like readInteger, but an empty line keeps the old value. */
bool WorkflowApp::readIntegerOrKeep(const char* prompt, int minimum, int maximum, int& value)
{
    char buffer[INPUT_BUFFER_SIZE];
    while (readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
        if (buffer[0] == '\0') {
            return true;   /* keep the current value */
        }
        int number = 0;
        if (pu_parse_int(buffer, &number) && number >= minimum && number <= maximum) {
            value = number;
            return true;
        }
        std::cout << "  Please enter a whole number from " << minimum << " to " << maximum
                  << ", or press Enter to keep it.\n";
    }
    return false;
}

bool WorkflowApp::readTime(const char* prompt, int& minutes)
{
    char buffer[INPUT_BUFFER_SIZE];
    while (readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
        if (pu_parse_time(buffer, &minutes)) {
            return true;
        }
        std::cout << "  Please type a time like 14:10 (or 02:10 PM).\n";
    }
    return false;
}

/* ===================== Phase 3: planning ===================== */

void WorkflowApp::makeSchedulingInput(SchedulingInput& input) const
{
    input.tasks = taskManager.tasks;
    input.taskCount = tm_count(&taskManager);
    input.slots = sm_get_slots(&scheduleManager);
    input.slotCount = sm_slot_count(&scheduleManager);
    input.graph = &taskGraph;
    input.map = &locationGraph;
}

const TimeSlot* WorkflowApp::findSlot(int slotId) const
{
    const TimeSlot* slots = sm_get_slots(&scheduleManager);
    for (int index = 0; index < sm_slot_count(&scheduleManager); index++) {
        if (slots[index].slotId == slotId) {
            return &slots[index];
        }
    }
    return nullptr;
}

/*
 * Runs one planner. The parameter is a reference to the BASE class, so the
 * same function works for Greedy and for Knapsack: plan() calls the right
 * planSlots() at run time (virtual function).
 */
void WorkflowApp::runPlanner(const Scheduler& planner)
{
    SchedulingInput input;
    makeSchedulingInput(input);

    ScheduleResult result;
    if (!planner.plan(input, result)) {
        std::cout << "[X] The day cannot be planned.\n";
        checkDependencyCycles();
        return;
    }
    printPlanSummary(result);
    currentPlan = result;   /* safe copy: ScheduleResult has only fixed arrays */
    hasPlan = true;
    printOk("This is now your current plan. Choose 7 (View Itinerary) to see the timeline.");
}

void WorkflowApp::comparePlanners()
{
    SchedulingInput input;
    makeSchedulingInput(input);

    /* Two different algorithms behind the same base-class pointer. */
    const Scheduler* planners[2] = { &greedyPlanner, &knapsackPlanner };
    ScheduleResult results[2];
    for (int index = 0; index < 2; index++) {
        if (!planners[index]->plan(input, results[index])) {
            std::cout << "[X] The day cannot be planned.\n";
            checkDependencyCycles();
            return;
        }
        printPlanSummary(results[index]);
    }

    std::cout << "\n=== Greedy vs 0/1 Knapsack ===\n";
    printf("  %-24s %14s %14s\n", "", results[0].getAlgorithmName(), results[1].getAlgorithmName());
    printf("  %-24s %14d %14d\n", "Tasks planned", results[0].getPlannedCount(),
           results[1].getPlannedCount());
    printf("  %-24s %14d %14d\n", "Priority achieved", results[0].getAchievedPriority(),
           results[1].getAchievedPriority());
    printf("  %-24s %13.1f%% %13.1f%%\n", "Priority (% of possible)", results[0].getPriorityPercent(),
           results[1].getPriorityPercent());
    printf("  %-24s %14d %14d\n", "Task minutes", results[0].getTaskMinutes(),
           results[1].getTaskMinutes());
    printf("  %-24s %13.1f%% %13.1f%%\n", "Free time used for tasks", results[0].getTimeUsePercent(),
           results[1].getTimeUsePercent());
    printf("  %-24s %14d %14d\n", "Walking minutes", results[0].getWalkingMinutes(),
           results[1].getWalkingMinutes());
    printf("  %-24s %14d %14d\n", "Tasks not planned", results[0].getDeferredCount(),
           results[1].getDeferredCount());

    /* Better = more priority; if equal, more task minutes. */
    int better = -1;
    if (results[0].getAchievedPriority() != results[1].getAchievedPriority()) {
        better = (results[0].getAchievedPriority() > results[1].getAchievedPriority()) ? 0 : 1;
    } else if (results[0].getTaskMinutes() != results[1].getTaskMinutes()) {
        better = (results[0].getTaskMinutes() > results[1].getTaskMinutes()) ? 0 : 1;
    }
    if (better == -1) {
        std::cout << "Both planners reach the same priority and task time today.\n";
    } else {
        std::cout << results[better].getAlgorithmName() << " gives the better plan today.\n";
    }

    int keep = 0;
    if (!readInteger("Keep which plan? (1 = Greedy, 2 = Knapsack, 0 = neither): ", 0, 2, keep)
        || keep == 0) {
        return;
    }
    currentPlan = results[keep - 1];
    hasPlan = true;
    printOk("Plan kept. Choose 7 (View Itinerary) to see the timeline.");
}

void WorkflowApp::printPlanSummary(const ScheduleResult& plan) const
{
    char startText[8];
    char endText[8];

    std::cout << "\n=== Plan made by " << plan.getAlgorithmName() << " ===\n";
    for (int index = 0; index < plan.getSlotCount(); index++) {
        const SlotUsage& usage = plan.getSlot(index);
        const TimeSlot* slot = findSlot(usage.slotId);
        if (slot == nullptr) {
            continue;
        }
        pu_format_time(slot->startMinute, startText, sizeof(startText));
        pu_format_time(slot->endMinute, endText, sizeof(endText));
        int spare = usage.capacityMinutes - usage.taskMinutes - usage.walkingMinutes;
        printf("  Slot %d  %s-%s  (%d min: tasks %d, walking %d, free %d)\n", usage.slotId,
               startText, endText, usage.capacityMinutes, usage.taskMinutes,
               usage.walkingMinutes, spare);

        bool any = false;
        for (int item = 0; item < plan.getPlannedCount(); item++) {
            const PlannedTask& planned = plan.getPlanned(item);
            if (planned.slotId != usage.slotId) {
                continue;
            }
            const TaskData* task = tm_find_task(&taskManager, planned.taskId);
            if (task != nullptr) {
                printf("      T%-3d %-30s P%-2d %3d min\n", task->id, task->name, task->priority,
                       task->durationMinutes);
                any = true;
            }
        }
        if (!any) {
            printf("      (nothing fits)\n");
        }
    }

    printf("  Planned   : %d task(s)\n", plan.getPlannedCount());
    printf("  Priority  : %d of %d possible (%.1f%%)\n", plan.getAchievedPriority(),
           plan.getPossiblePriority(), plan.getPriorityPercent());
    printf("  Task time : %d of %d free minutes (%.1f%%)\n", plan.getTaskMinutes(),
           plan.getFreeMinutes(), plan.getTimeUsePercent());
    printf("  Walking   : %d min\n", plan.getWalkingMinutes());

    if (plan.getDeferredCount() > 0) {
        printf("  Not planned:\n");
        for (int index = 0; index < plan.getDeferredCount(); index++) {
            const DeferredTask& item = plan.getDeferred(index);
            printf("      T%-3d %-30s - %s\n", item.taskId, taskNameOf(item.taskId), item.reason);
        }
    }
}

/* Prints one walking step of the itinerary, with the buildings on the way. */
void WorkflowApp::printWalk(int startMinute, int fromId, int toId, int minutes) const
{
    char startText[8];
    char endText[8];
    pu_format_time(startMinute, startText, sizeof(startText));
    pu_format_time(startMinute + minutes, endText, sizeof(endText));
    printf("    %s-%s  WALK   ", startText, endText);

    DijkstraSolver solver(&locationGraph);
    ShortestPath path;
    if (solver.findPath(fromId, toId, path) == DIJKSTRA_OK) {
        for (int position = 0; position < path.length; position++) {
            printf("%s%s", position > 0 ? " -> " : "", lg_name_of(&locationGraph, path.locationIds[position]));
        }
    } else {
        printf("%s -> %s", lg_name_of(&locationGraph, fromId), lg_name_of(&locationGraph, toId));
    }
    printf("  (%d min)\n", minutes);
}

void WorkflowApp::showItinerary() const
{
    if (!hasPlan) {
        std::cout << "No plan yet. Choose 6 (Optimize My Day) first.\n";
        return;
    }

    char startText[8];
    char endText[8];
    std::cout << "Your day (plan made by " << currentPlan.getAlgorithmName() << ")\n\n";

    for (int index = 0; index < sm_period_count(&scheduleManager); index++) {
        const Period* period = sm_get_period(&scheduleManager, index);
        pu_format_time(period->startMinute, startText, sizeof(startText));
        pu_format_time(period->endMinute, endText, sizeof(endText));

        if (period->type == PERIOD_CLASS) {
            printf("%s-%s  CLASS  %s @ %s\n", startText, endText, period->name,
                   lg_name_of(&locationGraph, period->locationId));
            continue;
        }

        /* A free period: find its slot and how the plan uses it. */
        const TimeSlot* slot = nullptr;
        const TimeSlot* slots = sm_get_slots(&scheduleManager);
        for (int s = 0; s < sm_slot_count(&scheduleManager); s++) {
            if (slots[s].startMinute == period->startMinute) {
                slot = &slots[s];
            }
        }
        const SlotUsage* usage = nullptr;
        for (int s = 0; slot != nullptr && s < currentPlan.getSlotCount(); s++) {
            if (currentPlan.getSlot(s).slotId == slot->slotId) {
                usage = &currentPlan.getSlot(s);
            }
        }
        printf("%s-%s  FREE   %s\n", startText, endText, period->name);
        if (slot == nullptr || usage == nullptr) {
            continue;
        }

        int clock = slot->startMinute;
        for (int item = 0; item < currentPlan.getPlannedCount(); item++) {
            const PlannedTask& planned = currentPlan.getPlanned(item);
            if (planned.slotId != slot->slotId) {
                continue;
            }
            const TaskData* task = tm_find_task(&taskManager, planned.taskId);
            if (task == nullptr) {
                continue;
            }
            if (planned.walkMinutes > 0) {
                printWalk(planned.startMinute - planned.walkMinutes, planned.walkFromLocationId,
                          task->locationId, planned.walkMinutes);
            }
            pu_format_time(planned.startMinute, startText, sizeof(startText));
            pu_format_time(planned.endMinute, endText, sizeof(endText));
            printf("    %s-%s  TASK   T%d %s (P%d) @ %s\n", startText, endText, task->id, task->name,
                   task->priority, lg_name_of(&locationGraph, task->locationId));
            clock = planned.endMinute;
        }
        if (usage->walkToClassMinutes > 0) {
            printWalk(clock, usage->endLocationId, slot->toLocationId, usage->walkToClassMinutes);
            clock += usage->walkToClassMinutes;
        }
        if (slot->endMinute > clock) {
            printf("    %d min left free\n", slot->endMinute - clock);
        }
    }

    if (currentPlan.getDeferredCount() > 0) {
        std::cout << "\nNot planned today:\n";
        for (int index = 0; index < currentPlan.getDeferredCount(); index++) {
            const DeferredTask& item = currentPlan.getDeferred(index);
            printf("  T%-3d %-30s - %s\n", item.taskId, taskNameOf(item.taskId), item.reason);
        }
    }
}

/* ===================== Phase 3: editing ===================== */

/* Any edit makes the old plan out of date and needs saving. */
void WorkflowApp::markDataChanged()
{
    hasUnsavedChanges = true;
    if (hasPlan) {
        hasPlan = false;
        std::cout << "  (The old plan no longer matches the data. Run 6 Optimize My Day again.)\n";
    }
}

void WorkflowApp::addLocation()
{
    char name[INPUT_BUFFER_SIZE];
    if (!readLine("New location name: ", name, INPUT_BUFFER_SIZE)) {
        return;
    }
    if (std::strchr(name, FIELD_SEPARATOR) != nullptr) {
        std::cout << "[X] A name cannot contain '|'.\n";
        return;
    }
    int id = lg_next_id(&locationGraph);
    LocationGraphStatus status = lg_add_location(&locationGraph, id, name);
    if (status != LG_OK) {
        std::cout << "[X] Not added: " << lg_status_message(status) << ".\n";
        return;
    }
    std::cout << "[OK] Added location " << id << " " << name
              << ". Add a route (Manage Routes) so it can be reached.\n";
    markDataChanged();
}

void WorkflowApp::addRoute()
{
    std::cout << "Locations:\n";
    printLocationList();
    int fromId = 0;
    int toId = 0;
    int minutes = 0;
    if (!readLocationId("From (ID or name): ", fromId) || !readLocationId("To   (ID or name): ", toId)
        || !readInteger("Walking minutes (0-600): ", 0, 600, minutes)) {
        return;
    }
    LocationGraphStatus status = lg_add_route(&locationGraph, fromId, toId, minutes);
    if (status != LG_OK) {
        std::cout << "[X] Not added: " << lg_status_message(status) << ".\n";
        return;
    }
    printOk("Route added (both directions).");
    markDataChanged();
}

void WorkflowApp::removeRoute()
{
    showLocationGraph();
    int fromId = 0;
    int toId = 0;
    if (!readLocationId("From (ID or name): ", fromId) || !readLocationId("To   (ID or name): ", toId)) {
        return;
    }
    LocationGraphStatus status = lg_remove_route(&locationGraph, fromId, toId);
    if (status != LG_OK) {
        std::cout << "[X] Not removed: " << lg_status_message(status) << ".\n";
        return;
    }
    printOk("Route removed (both directions).");
    markDataChanged();
}

void WorkflowApp::addPeriod()
{
    Period period;
    std::memset(&period, 0, sizeof(period));
    period.id = sm_next_id(&scheduleManager);

    char name[INPUT_BUFFER_SIZE];
    int type = 0;
    if (!readTime("Start time (HH:MM): ", period.startMinute)
        || !readTime("End time   (HH:MM): ", period.endMinute)
        || !readLine("Name (e.g. Free Period or a subject): ", name, INPUT_BUFFER_SIZE)
        || !readInteger("Type (1 = CLASS, 2 = FREE): ", 1, 2, type)) {
        return;
    }
    if (name[0] == '\0' || std::strchr(name, FIELD_SEPARATOR) != nullptr
        || !pu_copy_text(period.name, MAX_NAME_LEN, name)) {
        std::cout << "[X] The name must not be empty, too long, or contain '|'.\n";
        return;
    }

    if (type == 1) {
        period.type = PERIOD_CLASS;
        std::cout << "Locations:\n";
        printLocationList();
        if (!readLocationId("Where is the class (ID or name): ", period.locationId)) {
            return;
        }
    } else {
        period.type = PERIOD_FREE;
        period.locationId = NO_LOCATION;
    }

    ScheduleStatus status = sm_add_period(&scheduleManager, &period);
    if (status != SM_OK) {
        std::cout << "[X] Not added: " << sm_status_message(status) << ".\n";
        return;
    }
    std::cout << "[OK] Period " << period.id << " added. Free slots now: "
              << sm_slot_count(&scheduleManager) << ".\n";
    markDataChanged();
}

void WorkflowApp::deletePeriod()
{
    char startText[8];
    char endText[8];
    std::cout << "Periods:\n";
    for (int index = 0; index < sm_period_count(&scheduleManager); index++) {
        const Period* period = sm_get_period(&scheduleManager, index);
        pu_format_time(period->startMinute, startText, sizeof(startText));
        pu_format_time(period->endMinute, endText, sizeof(endText));
        printf("  [%d] %s-%s %-5s %s\n", period->id, startText, endText,
               period->type == PERIOD_CLASS ? "CLASS" : "FREE", period->name);
    }
    int periodId = 0;
    if (!readInteger("Period ID to delete (0 = cancel): ", 0, 1000000, periodId) || periodId == 0) {
        return;
    }
    ScheduleStatus status = sm_delete_period(&scheduleManager, periodId);
    if (status != SM_OK) {
        std::cout << "[X] Not deleted: " << sm_status_message(status) << ".\n";
        return;
    }
    printOk("Period deleted.");
    markDataChanged();
}

/*
 * Asks for every field of a task. When keepAllowed is true (update),
 * pressing Enter keeps the current value. Returns false only if input ended.
 */
bool WorkflowApp::readTaskFields(TaskData& task, bool keepAllowed)
{
    char buffer[INPUT_BUFFER_SIZE];
    char prompt[160];
    const char* keep = keepAllowed ? ", Enter = keep" : "";

    /* Name. When updating, the prompt shows the current value in [ ]. */
    while (true) {
        if (keepAllowed) {
            snprintf(prompt, sizeof(prompt), "Name [%s, Enter = keep]: ", task.name);
        } else {
            snprintf(prompt, sizeof(prompt), "Name: ");
        }
        if (!readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
            return false;
        }
        if (buffer[0] == '\0' && keepAllowed) {
            break;
        }
        if (buffer[0] != '\0' && std::strchr(buffer, FIELD_SEPARATOR) == nullptr
            && pu_copy_text(task.name, MAX_NAME_LEN, buffer)) {
            break;
        }
        std::cout << "  The name must not be empty, longer than 63 characters, or contain '|'.\n";
    }

    /* Priority and duration */
    if (keepAllowed) {
        snprintf(prompt, sizeof(prompt), "Priority 1-10 [%d, Enter = keep]: ", task.priority);
    } else {
        snprintf(prompt, sizeof(prompt), "Priority 1-10: ");
    }
    bool ok = keepAllowed ? readIntegerOrKeep(prompt, MIN_PRIORITY, MAX_PRIORITY, task.priority)
                          : readInteger(prompt, MIN_PRIORITY, MAX_PRIORITY, task.priority);
    if (!ok) {
        return false;
    }
    if (keepAllowed) {
        snprintf(prompt, sizeof(prompt), "Duration in minutes 1-600 [%d, Enter = keep]: ",
                 task.durationMinutes);
    } else {
        snprintf(prompt, sizeof(prompt), "Duration in minutes 1-600: ");
    }
    ok = keepAllowed ? readIntegerOrKeep(prompt, MIN_TASK_DURATION, MAX_TASK_DURATION, task.durationMinutes)
                     : readInteger(prompt, MIN_TASK_DURATION, MAX_TASK_DURATION, task.durationMinutes);
    if (!ok) {
        return false;
    }

    /* Location */
    std::cout << "Locations:\n";
    printLocationList();
    while (true) {
        if (keepAllowed) {
            snprintf(prompt, sizeof(prompt), "Location ID or name [%s, Enter = keep]: ",
                     lg_name_of(&locationGraph, task.locationId));
        } else {
            snprintf(prompt, sizeof(prompt), "Location ID or name: ");
        }
        if (!readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
            return false;
        }
        if (buffer[0] == '\0' && keepAllowed) {
            break;
        }
        int number = 0;
        int index = -1;
        if (pu_parse_int(buffer, &number)) {
            index = lg_find_index_by_id(&locationGraph, number);
        } else if (buffer[0] != '\0') {
            index = lg_find_index_by_name(&locationGraph, buffer);
        }
        if (index != -1) {
            task.locationId = lg_get_id(&locationGraph, index);
            break;
        }
        std::cout << "  Unknown location. Enter an ID from the list or the exact name.\n";
    }

    /* Prerequisites: "-" or IDs like 1,4 */
    while (true) {
        snprintf(prompt, sizeof(prompt), "Prerequisite task IDs, e.g. 1,4 (- = none%s): ", keep);
        if (!readLine(prompt, buffer, INPUT_BUFFER_SIZE)) {
            return false;
        }
        if (buffer[0] == '\0' && keepAllowed) {
            break;
        }
        if (std::strcmp(buffer, "-") == 0) {
            task.prerequisiteCount = 0;
            break;
        }

        char* parts[MAX_PREREQUISITES + 1];
        int partCount = pu_split(buffer, ',', parts, MAX_PREREQUISITES + 1);
        const char* problem = nullptr;
        int ids[MAX_PREREQUISITES];
        if (partCount > MAX_PREREQUISITES) {
            problem = "at most 5 prerequisites";
        }
        for (int position = 0; problem == nullptr && position < partCount; position++) {
            if (!pu_parse_int(parts[position], &ids[position])) {
                problem = "use numbers separated by commas";
            } else if (ids[position] == task.id) {
                problem = "a task cannot need itself";
            } else if (tm_find_task(&taskManager, ids[position]) == nullptr) {
                problem = "that task ID does not exist";
            }
            for (int earlier = 0; problem == nullptr && earlier < position; earlier++) {
                if (ids[earlier] == ids[position]) {
                    problem = "the same ID is listed twice";
                }
            }
        }
        if (problem == nullptr) {
            task.prerequisiteCount = partCount;
            for (int position = 0; position < partCount; position++) {
                task.prerequisiteIds[position] = ids[position];
            }
            break;
        }
        std::cout << "  " << problem << ".\n";
    }
    return true;
}

/*
 * After a task was added or changed: rebuild the graph and make sure no
 * dependency cycle appeared. If one did, undo the change.
 */
bool WorkflowApp::applyTaskGraphChange(const TaskData* undoRecord, int addedTaskId)
{
    rebuildTaskGraph(false);

    int cycle[MAX_TASKS + 1];
    int cycleLength = 0;
    if (tg_find_cycle(&taskGraph, cycle, &cycleLength) == 1) {
        std::cout << "[X] Change undone: it would create a dependency cycle (";
        for (int position = 0; position < cycleLength; position++) {
            std::cout << "T" << cycle[position] << (position + 1 < cycleLength ? " -> " : ")\n");
        }
        if (addedTaskId > 0) {
            tm_delete_task(&taskManager, addedTaskId);
        } else if (undoRecord != nullptr) {
            tm_update_task(&taskManager, undoRecord);
        }
        rebuildTaskGraph(false);
        rebuildTaskObjects();
        return false;
    }
    rebuildTaskObjects();
    return true;
}

void WorkflowApp::addTask()
{
    TaskData task;
    std::memset(&task, 0, sizeof(task));
    task.id = tm_next_id(&taskManager);
    task.priority = 5;
    task.durationMinutes = 30;
    task.locationId = NO_LOCATION;

    std::cout << "New task T" << task.id << "\n";
    if (!readTaskFields(task, false)) {
        return;
    }
    const char* problem = tm_validation_error(&task);
    if (problem != nullptr) {
        std::cout << "[X] Not added: " << problem << ".\n";
        return;
    }
    TaskManagerStatus status = tm_add_task(&taskManager, &task);
    if (status != TM_OK) {
        std::cout << "[X] Not added: " << tm_status_message(status) << ".\n";
        return;
    }
    if (applyTaskGraphChange(nullptr, task.id)) {
        std::cout << "[OK] Added T" << task.id << " " << task.name << ".\n";
        markDataChanged();
    }
}

void WorkflowApp::updateTask()
{
    showAllTasks();
    int taskId = 0;
    if (!readInteger("Task ID to update (0 = cancel): ", 0, 1000000, taskId) || taskId == 0) {
        return;
    }
    const TaskData* found = tm_find_task(&taskManager, taskId);
    if (found == nullptr) {
        std::cout << "[X] No task has ID " << taskId << ".\n";
        return;
    }
    TaskData before = *found;   /* kept so the change can be undone */
    TaskData task = *found;
    if (!readTaskFields(task, true)) {
        return;
    }
    const char* problem = tm_validation_error(&task);
    if (problem != nullptr) {
        std::cout << "[X] Not changed: " << problem << ".\n";
        return;
    }
    tm_update_task(&taskManager, &task);
    if (applyTaskGraphChange(&before, 0)) {
        std::cout << "[OK] T" << task.id << " updated.\n";
        markDataChanged();
    }
}

void WorkflowApp::markTaskDone()
{
    showPendingTasks();
    int taskId = 0;
    if (!readInteger("Task ID that is done (0 = cancel): ", 0, 1000000, taskId) || taskId == 0) {
        return;
    }
    const TaskData* task = tm_find_task(&taskManager, taskId);
    if (task == nullptr) {
        std::cout << "[X] No task has ID " << taskId << ".\n";
        return;
    }
    if (task->isCompleted) {
        std::cout << "T" << taskId << " is already DONE.\n";
        return;
    }
    tm_mark_completed(&taskManager, taskId);
    rebuildTaskObjects();
    std::cout << "[OK] T" << taskId << " marked as DONE.\n";
    markDataChanged();
}

void WorkflowApp::deleteTask()
{
    showAllTasks();
    int taskId = 0;
    if (!readInteger("Task ID to delete (0 = cancel): ", 0, 1000000, taskId) || taskId == 0) {
        return;
    }
    TaskManagerStatus status = tm_delete_task(&taskManager, taskId);
    if (status == TM_ERR_HAS_DEPENDENTS) {
        std::cout << "[X] Not deleted: another task needs T" << taskId
                  << ". Remove it from that task's prerequisites first (Update a task).\n";
        return;
    }
    if (status != TM_OK) {
        std::cout << "[X] Not deleted: " << tm_status_message(status) << ".\n";
        return;
    }
    rebuildTaskGraph(false);
    rebuildTaskObjects();
    std::cout << "[OK] T" << taskId << " deleted.\n";
    markDataChanged();
}

void WorkflowApp::showTaskOrder() const
{
    int order[MAX_TASKS];
    int count = 0;
    TaskGraphStatus status = tg_topological_sort(&taskGraph, order, &count);
    if (status == TG_ERR_CYCLE) {
        std::cout << "No valid order exists, because of a dependency cycle.\n";
        checkDependencyCycles();
        return;
    }
    if (status != TG_OK) {
        std::cout << "Could not sort the tasks: " << tg_status_message(status) << ".\n";
        return;
    }

    std::cout << "Valid task order (Kahn's algorithm): every task comes after its prerequisites.\n"
              << "When several tasks are ready, the one with the higher priority comes first.\n";
    for (int position = 0; position < count; position++) {
        const TaskData* task = tm_find_task(&taskManager, order[position]);
        if (task != nullptr) {
            printf("  %2d. T%-3d %-30s P%-2d%s\n", position + 1, task->id, task->name,
                   task->priority, task->isCompleted ? "  [DONE]" : "");
        }
    }
}

/* ===================== Phase 3: saving ===================== */

void WorkflowApp::saveData()
{
    bool allSaved = true;
    if (!lg_save_locations(&locationGraph, LOCATIONS_FILE)) {
        std::cout << "[X] Could not write " << LOCATIONS_FILE << "\n";
        allSaved = false;
    }
    if (!lg_save_routes(&locationGraph, ROUTES_FILE)) {
        std::cout << "[X] Could not write " << ROUTES_FILE << "\n";
        allSaved = false;
    }
    if (!sm_save_to_file(&scheduleManager, SCHEDULE_FILE)) {
        std::cout << "[X] Could not write " << SCHEDULE_FILE << "\n";
        allSaved = false;
    }
    if (!tm_save_to_file(&taskManager, TASKS_FILE)) {
        std::cout << "[X] Could not write " << TASKS_FILE << "\n";
        allSaved = false;
    }
    if (allSaved) {
        printOk("All data saved to the data/ folder.");
        hasUnsavedChanges = false;
    } else {
        std::cout << "[!] Some files were not saved. Run the program from the project folder.\n";
    }
}
