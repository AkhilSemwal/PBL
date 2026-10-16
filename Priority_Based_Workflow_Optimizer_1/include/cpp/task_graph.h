/*
 * task_graph.h
 * Owner: Samriddhi
 *
 * Some tasks can only be started after another task is finished
 * (for example, we revise the heap notes before solving the heap
 * practice set). This file stores those "do this first" rules as a
 * directed graph, using adjacency lists.
 *
 * How we draw the arrows:
 *   If task B needs task A, we store the arrow A -> B.
 *   Read it as "A has to be finished before B".
 *   So the list of A contains the tasks that are waiting for A.
 *
 * The graph only knows the rules, not who has finished what.
 * Whether a task is done or pending is kept in the task list itself.
 */

#ifndef TASK_GRAPH_H
#define TASK_GRAPH_H

#include "task.h"   /* MAX_TASKS */

/* Every function that can fail returns one of these codes. */
typedef enum TaskGraphStatus
{
    TG_OK = 0,
    TG_ERR_INVALID_ID,     /* the task ID is not in the graph (or is below 1) */
    TG_ERR_DUPLICATE,      /* this task or this arrow is already there        */
    TG_ERR_SELF_LOOP,      /* a task cannot wait for itself                   */
    TG_ERR_FULL,           /* we already have MAX_TASKS tasks                 */
    TG_ERR_NO_MEMORY,      /* malloc could not give us memory                 */
    TG_ERR_CYCLE           /* the rules go round in a circle                  */
} TaskGraphStatus;

/* One box in a linked list: "the task at toIndex is waiting for me". */
typedef struct DependencyNode
{
    int toIndex;
    struct DependencyNode* next;
} DependencyNode;

/* One task in the graph, with the list of tasks that depend on it. */
typedef struct TaskVertex
{
    int taskId;
    int priority;                 /* copied from the task; Phase 3 uses it for ordering */
    DependencyNode* dependents;   /* first box of this task's list                      */
} TaskVertex;

typedef struct TaskGraph
{
    TaskVertex vertices[MAX_TASKS];
    int count;                    /* how many tasks are in the graph right now */
} TaskGraph;

/* Start with an empty graph. */
void tg_init(TaskGraph* graph);

/* Give back the memory of every list. Calling it twice does no harm. */
void tg_free(TaskGraph* graph);

/* Put a task into the graph (with no arrows yet). */
TaskGraphStatus tg_add_task(TaskGraph* graph, int taskId, int priority);

/* Add the arrow prerequisiteId -> taskId, i.e. "taskId needs prerequisiteId". */
TaskGraphStatus tg_add_prerequisite(TaskGraph* graph, int prerequisiteId, int taskId);

/* Where is this task stored? Returns its index, or -1 if we don't have it. */
int tg_find_index(const TaskGraph* graph, int taskId);

/* How many tasks and how many arrows the graph has. */
int tg_vertex_count(const TaskGraph* graph);
int tg_edge_count(const TaskGraph* graph);

/*
 * Which tasks does taskId need? Their IDs are written into out
 * (at most maxCount of them) and the function returns how many it found.
 */
int tg_get_prerequisites(const TaskGraph* graph, int taskId, int* out, int maxCount);

/* Print every task, the tasks waiting for it, and the tasks it needs. */
void tg_display(const TaskGraph* graph);

/*
 * Looks for a circle in the rules using DFS (depth-first search).
 * Example of a circle: T1 needs T2 and T2 needs T1 - neither can ever start.
 *
 * Returns  1 if a circle is found, and writes it into cycleIds,
 *            e.g. {1, 2, 1} for T1 -> T2 -> T1 (cycleLength = 3).
 *            cycleIds must have room for MAX_TASKS + 1 numbers.
 * Returns  0 if there is no circle.
 * Returns -1 if we ran out of memory.
 */
int tg_find_cycle(const TaskGraph* graph, int* cycleIds, int* cycleLength);

/* Turns a status code into a short message we can print. */
const char* tg_status_message(TaskGraphStatus status);

/*
 * Coming in Phase 3:
 *   tg_prerequisites_done() - "has everything this task needs been finished?"
 *                             (the Greedy and Knapsack schedulers will ask this)
 *   tg_topological_sort()   - a valid order for all tasks, using Kahn's
 *                             algorithm and the max-heap from heap.h
 */

#endif /* TASK_GRAPH_H */
