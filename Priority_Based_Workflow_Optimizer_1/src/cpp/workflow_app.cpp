/*
 * workflow_app.cpp
 * Owner: Akhil (integration, console application)
 *
 * Connects all modules and implements the console menu.
 */
#include "workflow_app.h"
#include "dijkstra.h"
#include "task.h"

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
        snprintf(line, sizeof(line), " %-*s", BOX_WIDTH - 6, text);
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
    std::cout << "Goodbye! All memory has been released.\n";
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
        showPhase3Message("View Itinerary (whole day as a timeline)");
        break;
    case 8:
        showPhase3Message("Save Data");
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
}

/* ===================== sub-menus ===================== */

void WorkflowApp::showPhase3Message(const char* feature) const
{
    std::cout << "[Phase 3] " << feature
              << " will be added in the next phase.\n";
}

void WorkflowApp::manageTodo()
{
    static const char* const items[] = {
        "View all tasks",
        "View pending tasks",
        "View pending tasks by priority",
        "Show dependency graph",
        "Check for dependency cycles (DFS)",
        "Add / update / delete a task      (Phase 3)",
        "Show valid task order             (Phase 3)"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE TO-DO LIST", items, 7, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 7, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showAllTasks(); break;
        case 2: showPendingTasks(); break;
        case 3: showTasksByPriority(); break;
        case 4: showDependencyGraph(); break;
        case 5: checkDependencyCycles(); break;
        case 6: showPhase3Message("Adding, updating and deleting tasks"); break;
        default: showPhase3Message("Topological task order (Kahn's algorithm)"); break;
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
    std::cout << "In Phase 3 the Greedy scheduler will use this same order to fill the free slots.\n";
    delete[] order;
}

void WorkflowApp::manageLocations()
{
    static const char* const items[] = {
        "View all locations (campus map)",
        "Find shortest walking route (Dijkstra)",
        "Add a location                    (Phase 3)"
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
        default: showPhase3Message("Adding a location"); break;
        }
    }
}

void WorkflowApp::manageRoutes()
{
    static const char* const items[] = {
        "View all routes (campus map)",
        "Find shortest walking route (Dijkstra)",
        "Add / remove a route              (Phase 3)"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE ROUTES", items, 3, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 3, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showLocationGraph(); break;
        case 2: findShortestRoute(); break;
        default: showPhase3Message("Adding and removing routes"); break;
        }
    }
}

void WorkflowApp::manageSchedule()
{
    static const char* const items[] = {
        "View timetable and free slots",
        "Add / delete a period             (Phase 3)"
    };
    while (!inputClosed) {
        printMenuBox("MANAGE FIXED SCHEDULE", items, 2, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 2, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showTimetable(); break;
        default: showPhase3Message("Adding and deleting periods"); break;
        }
    }
}

void WorkflowApp::optimizeMyDay()
{
    static const char* const items[] = {
        "Plan with Greedy                  (Phase 3)",
        "Plan with 0/1 Knapsack DP         (Phase 3)",
        "Compare Greedy and Knapsack       (Phase 3)"
    };
    while (!inputClosed) {
        printMenuBox("OPTIMIZE MY DAY", items, 3, "Back");
        int choice = 0;
        if (!readInteger("> Choice: ", 0, 3, choice) || choice == 0) {
            return;
        }
        std::cout << "\n";
        switch (choice) {
        case 1: showPhase3Message("Greedy planning (optimizer.h/.cpp)"); break;
        case 2: showPhase3Message("0/1 Knapsack DP planning (knapsack.h/.cpp)"); break;
        default: showPhase3Message("Greedy vs Knapsack comparison"); break;
        }
    }
}
