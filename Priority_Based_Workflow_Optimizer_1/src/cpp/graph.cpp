/*
 * task_graph.cpp
 * Owner: Samriddhi
 *
 * The prerequisite graph: every task keeps a linked list of the tasks
 * that are waiting for it. We also check the rules for circles using
 * DFS, because a circle means some tasks can never be started.
 *
 * Written in C style (structures, pointers, linked lists and plain
 * functions), like our Data Structures lab programs.
 */

#include "task_graph.h"

#include <cstdio>
#include <cstdlib>

/*
 * The three states a task can be in while DFS is running.
 * The "visiting" state is what lets us spot a circle: if we reach a task
 * that is still on the path we are walking right now, we went round.
 */
#define STATE_UNVISITED 0   /* DFS has not reached this task yet           */
#define STATE_VISITING 1    /* on the path we are exploring at the moment  */
#define STATE_DONE 2        /* fully explored, nothing more to see here    */

void tg_init(TaskGraph* graph)
{
    graph->count = 0;
    for (int index = 0; index < MAX_TASKS; index++) {
        graph->vertices[index].taskId = 0;
        graph->vertices[index].priority = 0;
        graph->vertices[index].dependents = NULL;
    }
}

void tg_free(TaskGraph* graph)
{
    /* Each list was built with malloc, so we free it box by box. */
    for (int index = 0; index < graph->count; index++) {
        DependencyNode* node = graph->vertices[index].dependents;
        while (node != NULL) {
            DependencyNode* next = node->next;   /* remember the next box before freeing this one */
            free(node);
            node = next;
        }
        graph->vertices[index].dependents = NULL;
    }
    graph->count = 0;
}

int tg_find_index(const TaskGraph* graph, int taskId)
{
    /* Simple linear search - we have at most 100 tasks, so this is fast enough. */
    for (int index = 0; index < graph->count; index++) {
        if (graph->vertices[index].taskId == taskId) {
            return index;
        }
    }
    return -1;
}

TaskGraphStatus tg_add_task(TaskGraph* graph, int taskId, int priority)
{
    if (taskId < 1) {
        return TG_ERR_INVALID_ID;
    }
    if (tg_find_index(graph, taskId) != -1) {
        return TG_ERR_DUPLICATE;          /* the same task cannot be added twice */
    }
    if (graph->count >= MAX_TASKS) {
        return TG_ERR_FULL;
    }

    /* The new task goes into the next free place of the array. */
    TaskVertex* vertex = &graph->vertices[graph->count];
    vertex->taskId = taskId;
    vertex->priority = priority;
    vertex->dependents = NULL;            /* no arrows yet */
    graph->count++;
    return TG_OK;
}

TaskGraphStatus tg_add_prerequisite(TaskGraph* graph, int prerequisiteId, int taskId)
{
    if (prerequisiteId == taskId) {
        return TG_ERR_SELF_LOOP;          /* "T3 needs T3" makes no sense */
    }

    int fromIndex = tg_find_index(graph, prerequisiteId);
    int toIndex = tg_find_index(graph, taskId);
    if (fromIndex == -1 || toIndex == -1) {
        return TG_ERR_INVALID_ID;         /* both tasks must already exist */
    }

    /*
     * Go to the end of the prerequisite's list. On the way we also check
     * that this arrow is not already there, so we never store it twice.
     */
    DependencyNode* last = NULL;
    DependencyNode* node = graph->vertices[fromIndex].dependents;
    while (node != NULL) {
        if (node->toIndex == toIndex) {
            return TG_ERR_DUPLICATE;
        }
        last = node;
        node = node->next;
    }

    DependencyNode* newNode = (DependencyNode*)malloc(sizeof(DependencyNode));
    if (newNode == NULL) {
        return TG_ERR_NO_MEMORY;
    }
    newNode->toIndex = toIndex;
    newNode->next = NULL;

    /* Add at the end, so the list keeps the same order as the file. */
    if (last == NULL) {
        graph->vertices[fromIndex].dependents = newNode;   /* list was empty */
    } else {
        last->next = newNode;
    }
    return TG_OK;
}

int tg_vertex_count(const TaskGraph* graph)
{
    return graph->count;
}

int tg_edge_count(const TaskGraph* graph)
{
    /* Every box in every list is one arrow, so we just count the boxes. */
    int edges = 0;
    for (int index = 0; index < graph->count; index++) {
        const DependencyNode* node = graph->vertices[index].dependents;
        while (node != NULL) {
            edges++;
            node = node->next;
        }
    }
    return edges;
}

int tg_get_prerequisites(const TaskGraph* graph, int taskId, int* out, int maxCount)
{
    int targetIndex = tg_find_index(graph, taskId);
    if (targetIndex == -1) {
        return 0;
    }

    /*
     * Our lists point forward (from a task to the tasks waiting for it).
     * To find what taskId needs, we look through every list and pick the
     * tasks whose list contains taskId.
     */
    int found = 0;
    for (int index = 0; index < graph->count; index++) {
        const DependencyNode* node = graph->vertices[index].dependents;
        while (node != NULL) {
            if (node->toIndex == targetIndex && found < maxCount) {
                out[found] = graph->vertices[index].taskId;
                found++;
            }
            node = node->next;
        }
    }
    return found;
}

void tg_display(const TaskGraph* graph)
{
    printf("Dependency graph: %d task(s), %d prerequisite edge(s)\n",
           graph->count, tg_edge_count(graph));
    printf("Edge A -> B means: task A must be finished before task B.\n\n");

    if (graph->count == 0) {
        printf("  (graph is empty)\n");
        return;
    }

    for (int index = 0; index < graph->count; index++) {
        const TaskVertex* vertex = &graph->vertices[index];
        printf("  T%-3d (P%-2d) -> ", vertex->taskId, vertex->priority);

        /* First the tasks that are waiting for this one... */
        const DependencyNode* node = vertex->dependents;
        if (node == NULL) {
            printf("(no dependents)");
        }
        while (node != NULL) {
            printf("T%d", graph->vertices[node->toIndex].taskId);
            if (node->next != NULL) {
                printf(", ");
            }
            node = node->next;
        }

        /* ...then the tasks this one needs, if any. */
        int prerequisites[MAX_TASKS];
        int prerequisiteCount = tg_get_prerequisites(graph, vertex->taskId, prerequisites, MAX_TASKS);
        if (prerequisiteCount > 0) {
            printf("   | needs: ");
            for (int position = 0; position < prerequisiteCount; position++) {
                printf("T%d", prerequisites[position]);
                if (position + 1 < prerequisiteCount) {
                    printf(", ");
                }
            }
        }
        printf("\n");
    }
}

/*
 * One step of DFS, written with recursion.
 *
 * We mark the task as "visiting" and walk into every task waiting for it.
 * If we meet a task that is still "visiting", we have come back to our own
 * path - that is a circle. We remember where it starts and ends and stop.
 * When all neighbours are explored, the task becomes "done".
 */
static int dfs_visit(const TaskGraph* graph, int index, int* state, int* parent,
                     int* cycleStart, int* cycleEnd)
{
    state[index] = STATE_VISITING;

    const DependencyNode* node = graph->vertices[index].dependents;
    while (node != NULL) {
        int next = node->toIndex;
        if (state[next] == STATE_VISITING) {
            /* Came back to a task on the current path: circle found. */
            *cycleStart = next;
            *cycleEnd = index;
            return 1;
        }
        if (state[next] == STATE_UNVISITED) {
            parent[next] = index;   /* remember how we got here, to print the circle later */
            if (dfs_visit(graph, next, state, parent, cycleStart, cycleEnd)) {
                return 1;
            }
        }
        /* A task that is already "done" is safe - meeting it again is not a circle. */
        node = node->next;
    }

    state[index] = STATE_DONE;
    return 0;
}

int tg_find_cycle(const TaskGraph* graph, int* cycleIds, int* cycleLength)
{
    *cycleLength = 0;
    if (graph->count == 0) {
        return 0;                         /* no tasks, so no circle */
    }

    /* One state and one "came from" value for every task. */
    int* state = (int*)malloc((size_t)graph->count * sizeof(int));
    int* parent = (int*)malloc((size_t)graph->count * sizeof(int));
    if (state == NULL || parent == NULL) {
        free(state);
        free(parent);
        return -1;
    }

    for (int index = 0; index < graph->count; index++) {
        state[index] = STATE_UNVISITED;
        parent[index] = -1;
    }

    int cycleStart = -1;
    int cycleEnd = -1;
    int found = 0;

    /* Start a DFS from every task that nobody has reached yet. */
    for (int index = 0; index < graph->count && !found; index++) {
        if (state[index] == STATE_UNVISITED) {
            found = dfs_visit(graph, index, state, parent, &cycleStart, &cycleEnd);
        }
    }

    if (found) {
        /*
         * Build the circle so we can show it to the user.
         * Starting from where the circle closed, we follow the "came from"
         * links backwards until we reach the start of the circle.
         */
        int reversed[MAX_TASKS];
        int reversedCount = 0;
        int current = cycleEnd;
        while (current != cycleStart && current != -1 && reversedCount < MAX_TASKS) {
            reversed[reversedCount] = current;
            reversedCount++;
            current = parent[current];
        }

        /* Write it in the normal order and end with the start again, e.g. 1 -> 2 -> 1. */
        cycleIds[0] = graph->vertices[cycleStart].taskId;
        int written = 1;
        for (int position = reversedCount - 1; position >= 0; position--) {
            cycleIds[written] = graph->vertices[reversed[position]].taskId;
            written++;
        }
        cycleIds[written] = graph->vertices[cycleStart].taskId;
        written++;
        *cycleLength = written;
    }

    /* We are done with the helper arrays, so give the memory back. */
    free(state);
    free(parent);
    return found;
}

const char* tg_status_message(TaskGraphStatus status)
{
    switch (status) {
    case TG_OK:
        return "OK";
    case TG_ERR_INVALID_ID:
        return "task ID not found";
    case TG_ERR_DUPLICATE:
        return "duplicate (already exists)";
    case TG_ERR_SELF_LOOP:
        return "a task cannot depend on itself";
    case TG_ERR_FULL:
        return "graph is full";
    case TG_ERR_NO_MEMORY:
        return "out of memory";
    case TG_ERR_CYCLE:
        return "dependency cycle found";
    }
    return "unknown status";
}
