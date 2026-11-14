/*
 * task_graph.h
 * Owner: Samriddhi
 *
 * Directed graph of task prerequisites, stored as adjacency lists.
 *
 * Edge direction (used everywhere in the project):
 *   If task B requires task A, the graph stores the edge  A -> B.
 *   Read it as "A must finish before B".
 *   So the adjacency list of A holds its DEPENDENTS (tasks waiting for A).
 *
 * The graph only describes the ORDER rules. It does not know whether a
 * task is completed; callers pass the list of finished task IDs when they
 * ask whether a task's prerequisites are satisfied.
 */
#ifndef TASK_GRAPH_H
#define TASK_GRAPH_H

#include "task.h"   /* MAX_TASKS */

typedef enum TaskGraphStatus {
    TG_OK = 0,
    TG_ERR_INVALID_ID,     /* task ID is not in the graph (or not >= 1) */
    TG_ERR_DUPLICATE,      /* vertex or edge already exists             */
    TG_ERR_SELF_LOOP,      /* a task cannot depend on itself            */
    TG_ERR_FULL,           /* MAX_TASKS vertices already used           */
    TG_ERR_NO_MEMORY,      /* malloc failed                             */
    TG_ERR_CYCLE           /* topological order impossible: cycle found */
} TaskGraphStatus;

/* One node of an adjacency list: "the vertex at toIndex depends on me". */
typedef struct DependencyNode {
    int toIndex;
    struct DependencyNode* next;
} DependencyNode;

typedef struct TaskVertex {
    int taskId;
    int priority;                 /* copied from the task (used by Kahn's algorithm) */
    DependencyNode* dependents;   /* head of the adjacency list                    */
} TaskVertex;

typedef struct TaskGraph {
    TaskVertex vertices[MAX_TASKS];
    int count;
} TaskGraph;

void tg_init(TaskGraph* graph);

/* Frees every adjacency list and empties the graph. Safe to call twice. */
void tg_free(TaskGraph* graph);

/* Adds a vertex for a task. */
TaskGraphStatus tg_add_task(TaskGraph* graph, int taskId, int priority);

/* Adds the edge prerequisiteId -> taskId ("prerequisite before task"). */
TaskGraphStatus tg_add_prerequisite(TaskGraph* graph, int prerequisiteId, int taskId);

/* Returns the vertex index of taskId, or -1 if it is not in the graph. */
int tg_find_index(const TaskGraph* graph, int taskId);

int tg_vertex_count(const TaskGraph* graph);
int tg_edge_count(const TaskGraph* graph);

/* Writes the prerequisite IDs of taskId into out (at most maxCount). Returns how many. */
int tg_get_prerequisites(const TaskGraph* graph, int taskId, int* out, int maxCount);

/* Prints every task with the tasks that depend on it and the tasks it needs. */
void tg_display(const TaskGraph* graph);

/*
 * DFS cycle detection with three states (unvisited, visiting, done).
 * Returns 1 if a cycle exists and writes one cycle into cycleIds,
 * e.g. {7, 8, 7} for 7 -> 8 -> 7. *cycleLength receives the number of IDs
 * written. cycleIds must have room for MAX_TASKS + 1 values.
 * Returns 0 if the graph has no cycle, -1 if memory ran out.
 */
int tg_find_cycle(const TaskGraph* graph, int* cycleIds, int* cycleLength);

/* Converts a status code into a short English message. */
const char* tg_status_message(TaskGraphStatus status);


/* ------------------------------ Phase 3 ------------------------------ */

/*
 * Returns 1 if every prerequisite of taskId is in finishedIds
 * (the tasks that are DONE or already planned earlier), otherwise 0.
 * Used by the Greedy and Knapsack schedulers.
 */
int tg_prerequisites_done(const TaskGraph* graph, int taskId,
                          const int* finishedIds, int finishedCount);

/*
 * Kahn's algorithm. Writes every task ID into orderIds so that each task
 * comes after all its prerequisites. When several tasks are ready at the
 * same time, a MAX_HEAP picks the highest priority first (ties: the task
 * that comes first in the task list).
 * Returns TG_OK, TG_ERR_CYCLE (no valid order exists) or TG_ERR_NO_MEMORY.
 * orderIds must have room for MAX_TASKS values.
 */
TaskGraphStatus tg_topological_sort(const TaskGraph* graph, int* orderIds, int* orderCount);

#endif /* TASK_GRAPH_H */
