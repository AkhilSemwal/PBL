/*
 * task_graph.c
 * Owner: Samriddhi
 *
 * Prerequisite graph: adjacency lists, DFS cycle detection and
 * (Kahn's topological sort comes in Phase 3.)
 * Style: DS with C (structures, linked lists, pointers, functions)
 */
#include "task_graph.h"

#include <cstdio>
#include <cstdlib>

/* DFS colours for cycle detection. */
#define STATE_UNVISITED 0
#define STATE_VISITING 1   /* on the current DFS path */
#define STATE_DONE 2       /* fully processed         */

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
    for (int index = 0; index < graph->count; index++) {
        DependencyNode* node = graph->vertices[index].dependents;
        while (node != NULL) {
            DependencyNode* next = node->next;
            free(node);
            node = next;
        }
        graph->vertices[index].dependents = NULL;
    }
    graph->count = 0;
}

int tg_find_index(const TaskGraph* graph, int taskId)
{
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
        return TG_ERR_DUPLICATE;
    }
    if (graph->count >= MAX_TASKS) {
        return TG_ERR_FULL;
    }

    TaskVertex* vertex = &graph->vertices[graph->count];
    vertex->taskId = taskId;
    vertex->priority = priority;
    vertex->dependents = NULL;
    graph->count++;
    return TG_OK;
}

TaskGraphStatus tg_add_prerequisite(TaskGraph* graph, int prerequisiteId, int taskId)
{
    if (prerequisiteId == taskId) {
        return TG_ERR_SELF_LOOP;
    }

    int fromIndex = tg_find_index(graph, prerequisiteId);
    int toIndex = tg_find_index(graph, taskId);
    if (fromIndex == -1 || toIndex == -1) {
        return TG_ERR_INVALID_ID;
    }

    /* Walk to the end of the list, rejecting a duplicate edge on the way. */
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

    /* Append at the tail so lists keep the order in which edges were added. */
    if (last == NULL) {
        graph->vertices[fromIndex].dependents = newNode;
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

    /* A prerequisite of the target is any vertex whose list contains the target. */
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
 * Recursive DFS. Returns 1 as soon as a back edge (an edge into a VISITING
 * vertex) is found; *cycleStart and *cycleEnd then describe that edge.
 */
static int dfs_visit(const TaskGraph* graph, int index, int* state, int* parent,
                     int* cycleStart, int* cycleEnd)
{
    state[index] = STATE_VISITING;

    const DependencyNode* node = graph->vertices[index].dependents;
    while (node != NULL) {
        int next = node->toIndex;
        if (state[next] == STATE_VISITING) {
            /* Back edge index -> next closes a cycle. */
            *cycleStart = next;
            *cycleEnd = index;
            return 1;
        }
        if (state[next] == STATE_UNVISITED) {
            parent[next] = index;
            if (dfs_visit(graph, next, state, parent, cycleStart, cycleEnd)) {
                return 1;
            }
        }
        node = node->next;
    }

    state[index] = STATE_DONE;
    return 0;
}

int tg_find_cycle(const TaskGraph* graph, int* cycleIds, int* cycleLength)
{
    *cycleLength = 0;
    if (graph->count == 0) {
        return 0;
    }

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

    for (int index = 0; index < graph->count && !found; index++) {
        if (state[index] == STATE_UNVISITED) {
            found = dfs_visit(graph, index, state, parent, &cycleStart, &cycleEnd);
        }
    }

    if (found) {
        /* Walk back from cycleEnd to cycleStart using parent links. */
        int reversed[MAX_TASKS];
        int reversedCount = 0;
        int current = cycleEnd;
        while (current != cycleStart && current != -1 && reversedCount < MAX_TASKS) {
            reversed[reversedCount] = current;
            reversedCount++;
            current = parent[current];
        }

        /* Output: start, ..., end, start  (e.g. 7 -> 8 -> 7). */
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
