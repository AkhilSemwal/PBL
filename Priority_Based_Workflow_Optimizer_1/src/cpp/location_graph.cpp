/*
 * location_graph.cpp
 * Owner: Samriddhi
 *
 * The campus map: a fixed array of buildings, where every building keeps
 * a linked list of its neighbours and the walking time to each one.
 * The map is filled from locations.txt and routes.txt.
 *
 * Written in C style (structures, pointers, linked lists and plain
 * functions), like our Data Structures lab programs.
 */

#include "location_graph.h"
#include "task.h"   /* text helpers: pu_read_line, pu_split, pu_parse_int ... */

#include <cstdio>
#include <cstdlib>

void lg_init(LocationGraph* graph)
{
    graph->count = 0;
    for (int index = 0; index < MAX_LOCATIONS; index++) {
        graph->vertices[index].id = 0;
        graph->vertices[index].name[0] = '\0';
        graph->vertices[index].routes = NULL;   /* no neighbours yet */
    }
}

void lg_free(LocationGraph* graph)
{
    /* Every box in every list came from malloc, so we free them one by one. */
    for (int index = 0; index < graph->count; index++) {
        RouteNode* node = graph->vertices[index].routes;
        while (node != NULL) {
            RouteNode* next = node->next;   /* save the next box before freeing this one */
            free(node);
            node = next;
        }
        graph->vertices[index].routes = NULL;
    }
    graph->count = 0;   /* a second call now finds nothing to free */
}

int lg_find_index_by_id(const LocationGraph* graph, int id)
{
    /* Simple linear search - at most 50 buildings, so this is quick enough. */
    for (int index = 0; index < graph->count; index++) {
        if (graph->vertices[index].id == id) {
            return index;
        }
    }
    return -1;
}

int lg_find_index_by_name(const LocationGraph* graph, const char* name)
{
    /* Same idea, but compares names without caring about capital letters. */
    for (int index = 0; index < graph->count; index++) {
        if (pu_equals_ignore_case(graph->vertices[index].name, name)) {
            return index;
        }
    }
    return -1;
}

LocationGraphStatus lg_add_location(LocationGraph* graph, int id, const char* name)
{
    if (id < 1) {
        return LG_ERR_INVALID_ID;
    }
    if (name == NULL || name[0] == '\0') {
        return LG_ERR_BAD_NAME;
    }
    /* Two buildings may not share an ID or a name (e.g. "Library" and "library"). */
    if (lg_find_index_by_id(graph, id) != -1) {
        return LG_ERR_DUPLICATE;
    }
    if (lg_find_index_by_name(graph, name) != -1) {
        return LG_ERR_DUPLICATE;
    }
    if (graph->count >= MAX_LOCATIONS) {
        return LG_ERR_FULL;
    }

    /* The new building goes into the next free place of the array. */
    LocationVertex* vertex = &graph->vertices[graph->count];
    if (!pu_copy_text(vertex->name, MAX_NAME_LEN, name)) {
        return LG_ERR_BAD_NAME;   /* name does not fit in 63 characters */
    }
    vertex->id = id;
    vertex->routes = NULL;
    graph->count++;
    return LG_OK;
}

/* Is there already a path from the building at fromIndex to the one at toIndex? */
static int route_exists(const LocationGraph* graph, int fromIndex, int toIndex)
{
    const RouteNode* node = graph->vertices[fromIndex].routes;
    while (node != NULL) {
        if (node->toIndex == toIndex) {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

/*
 * Put a new box at the end of a building's list.
 * "link" points at the pointer we may have to change: first the list head,
 * then each box's next. When it points at a NULL, that is the end.
 */
static void append_route(LocationGraph* graph, int fromIndex, RouteNode* newNode)
{
    RouteNode** link = &graph->vertices[fromIndex].routes;
    while (*link != NULL) {
        link = &(*link)->next;
    }
    *link = newNode;
}

LocationGraphStatus lg_add_route(LocationGraph* graph, int idA, int idB, int walkingMinutes)
{
    if (walkingMinutes < 0) {
        return LG_ERR_NEGATIVE_WEIGHT;   /* Dijkstra would give wrong answers with negative times */
    }
    if (idA == idB) {
        return LG_ERR_SELF_LOOP;         /* a path from a building to itself is useless */
    }

    int indexA = lg_find_index_by_id(graph, idA);
    int indexB = lg_find_index_by_id(graph, idB);
    if (indexA == -1 || indexB == -1) {
        return LG_ERR_INVALID_ID;        /* both buildings must already be on the map */
    }
    /*
     * Every path is stored in both lists, so checking A's list is enough
     * to catch "1|2|5" and later "2|1|7" as the same path.
     */
    if (route_exists(graph, indexA, indexB)) {
        return LG_ERR_DUPLICATE;
    }

    /*
     * A two-way path needs two boxes: one in A's list pointing to B,
     * and one in B's list pointing to A. We get memory for both first, so
     * if malloc fails we never end up with only half a path.
     */
    RouteNode* forward = (RouteNode*)malloc(sizeof(RouteNode));
    RouteNode* backward = (RouteNode*)malloc(sizeof(RouteNode));
    if (forward == NULL || backward == NULL) {
        free(forward);    /* free(NULL) is allowed, so this is safe */
        free(backward);
        return LG_ERR_NO_MEMORY;
    }

    forward->toIndex = indexB;
    forward->walkingMinutes = walkingMinutes;
    forward->next = NULL;

    backward->toIndex = indexA;
    backward->walkingMinutes = walkingMinutes;
    backward->next = NULL;

    append_route(graph, indexA, forward);
    append_route(graph, indexB, backward);
    return LG_OK;
}

int lg_vertex_count(const LocationGraph* graph)
{
    return graph->count;
}

int lg_route_count(const LocationGraph* graph)
{
    /* Count all boxes in all lists. */
    int entries = 0;
    for (int index = 0; index < graph->count; index++) {
        const RouteNode* node = graph->vertices[index].routes;
        while (node != NULL) {
            entries++;
            node = node->next;
        }
    }
    /* Each two-way path was counted twice (once from each end). */
    return entries / 2;
}

int lg_get_id(const LocationGraph* graph, int index)
{
    if (index < 0 || index >= graph->count) {
        return NO_LOCATION;
    }
    return graph->vertices[index].id;
}

const char* lg_get_name(const LocationGraph* graph, int index)
{
    if (index < 0 || index >= graph->count) {
        return "Unknown location";   /* safe text instead of reading outside the array */
    }
    return graph->vertices[index].name;
}

const char* lg_name_of(const LocationGraph* graph, int id)
{
    /* Find the index first; a wrong ID gives -1, which lg_get_name handles. */
    return lg_get_name(graph, lg_find_index_by_id(graph, id));
}

const RouteNode* lg_first_route(const LocationGraph* graph, int index)
{
    if (index < 0 || index >= graph->count) {
        return NULL;
    }
    return graph->vertices[index].routes;
}

void showGraph(const LocationGraph* graph)
{
    printf("Campus map: %d location(s), %d walking route(s)\n\n",
           graph->count, lg_route_count(graph));

    if (graph->count == 0) {
        printf("  (no locations loaded)\n");
        return;
    }

    /* One line per building: its ID, its name, then every neighbour with the time. */
    for (int index = 0; index < graph->count; index++) {
        const LocationVertex* vertex = &graph->vertices[index];
        printf("  [%d] %-20s : ", vertex->id, vertex->name);

        const RouteNode* node = vertex->routes;
        if (node == NULL) {
            printf("(no routes - disconnected)");   /* e.g. the Research Block in our data */
        }
        while (node != NULL) {
            printf("%s (%d min)", graph->vertices[node->toIndex].name, node->walkingMinutes);
            if (node->next != NULL) {
                printf(", ");
            }
            node = node->next;
        }
        printf("\n");
    }
}

int lg_load_locations(LocationGraph* graph, const char* path, int* errorCount)
{
    *errorCount = 0;
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("  [ERROR] Cannot open locations file: %s\n", path);
        return -1;
    }

    char line[LINE_BUFFER_SIZE];
    int lineNumber = 0;
    int loaded = 0;
    int tooLong = 0;

    /* Read the file one line at a time; a bad line is skipped, not fatal. */
    while (pu_read_line(file, line, LINE_BUFFER_SIZE, &tooLong)) {
        lineNumber++;
        if (tooLong) {
            printf("  [WARN] %s line %d: line too long, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }
        if (pu_is_skippable(line)) {
            continue;   /* empty line or a '#' comment */
        }

        /* "2|Library" -> fields[0] = "2", fields[1] = "Library" */
        char* fields[2];
        int fieldCount = pu_split(line, FIELD_SEPARATOR, fields, 2);
        int id = 0;
        if (fieldCount != 2 || !pu_parse_int(fields[0], &id)) {
            printf("  [WARN] %s line %d: expected ID|Name, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }

        LocationGraphStatus status = lg_add_location(graph, id, fields[1]);
        if (status != LG_OK) {
            printf("  [WARN] %s line %d: location %d rejected (%s)\n",
                   path, lineNumber, id, lg_status_message(status));
            (*errorCount)++;
            continue;
        }
        loaded++;
    }

    fclose(file);
    return loaded;
}

int lg_load_routes(LocationGraph* graph, const char* path, int* errorCount)
{
    *errorCount = 0;
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("  [ERROR] Cannot open routes file: %s\n", path);
        return -1;
    }

    char line[LINE_BUFFER_SIZE];
    int lineNumber = 0;
    int loaded = 0;
    int tooLong = 0;

    while (pu_read_line(file, line, LINE_BUFFER_SIZE, &tooLong)) {
        lineNumber++;
        if (tooLong) {
            printf("  [WARN] %s line %d: line too long, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }
        if (pu_is_skippable(line)) {
            continue;
        }

        /* "1|2|5" -> from building 1 to building 2, 5 minutes. All three must be numbers. */
        char* fields[3];
        int fieldCount = pu_split(line, FIELD_SEPARATOR, fields, 3);
        int fromId = 0;
        int toId = 0;
        int minutes = 0;
        if (fieldCount != 3 || !pu_parse_int(fields[0], &fromId) ||
            !pu_parse_int(fields[1], &toId) || !pu_parse_int(fields[2], &minutes)) {
            printf("  [WARN] %s line %d: expected FromID|ToID|Minutes, skipped\n", path, lineNumber);
            (*errorCount)++;
            continue;
        }

        /* lg_add_route does the real checks (negative time, unknown building, duplicate ...). */
        LocationGraphStatus status = lg_add_route(graph, fromId, toId, minutes);
        if (status != LG_OK) {
            printf("  [WARN] %s line %d: route %d-%d rejected (%s)\n",
                   path, lineNumber, fromId, toId, lg_status_message(status));
            (*errorCount)++;
            continue;
        }
        loaded++;
    }

    fclose(file);
    return loaded;
}

const char* lg_status_message(LocationGraphStatus status)
{
    switch (status) {
    case LG_OK:
        return "OK";
    case LG_ERR_INVALID_ID:
        return "unknown or invalid location ID";
    case LG_ERR_DUPLICATE:
        return "duplicate location or route";
    case LG_ERR_NEGATIVE_WEIGHT:
        return "walking time cannot be negative";
    case LG_ERR_SELF_LOOP:
        return "route must connect two different locations";
    case LG_ERR_BAD_NAME:
        return "name is empty or too long";
    case LG_ERR_FULL:
        return "too many locations";
    case LG_ERR_NO_MEMORY:
        return "out of memory";
    }
    return "unknown status";
}
