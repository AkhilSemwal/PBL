/*
 * location_graph.h
 * Owner: Samriddhi
 *
 * This file holds our campus map. Every building (Library, Canteen,
 * Computer Lab ...) is a vertex, and every footpath between two
 * buildings is an edge with the walking time in minutes.
 *
 * Two things to remember about our map:
 *   - Paths are two-way: if you can walk from A to B in 5 minutes,
 *     you can also walk from B to A in 5 minutes (undirected graph).
 *   - Walking time can be 0 or more, never negative (weighted graph).
 *
 * We store it as an adjacency list: each building keeps a linked list
 * of its neighbours. Dijkstra reads the map only through the functions
 * below, so it never needs to know how the lists are built inside.
 */

#ifndef LOCATION_GRAPH_H
#define LOCATION_GRAPH_H

#include "task.h"   /* MAX_LOCATIONS and MAX_NAME_LEN */

/* Every function that can fail tells us why with one of these codes. */
typedef enum LocationGraphStatus
{
    LG_OK = 0,
    LG_ERR_INVALID_ID,       /* ID is below 1, or no building has this ID   */
    LG_ERR_DUPLICATE,        /* this building or this path already exists   */
    LG_ERR_NEGATIVE_WEIGHT,  /* walking time cannot be negative             */
    LG_ERR_SELF_LOOP,        /* a path must join two different buildings    */
    LG_ERR_BAD_NAME,         /* name is empty or longer than 63 characters  */
    LG_ERR_FULL,             /* we already have MAX_LOCATIONS buildings     */
    LG_ERR_NO_MEMORY         /* malloc could not give us memory             */
} LocationGraphStatus;

/* One box in a building's list: "from here you can walk to toIndex in walkingMinutes". */
typedef struct RouteNode
{
    int toIndex;             /* position of the neighbour in the vertices array */
    int walkingMinutes;
    struct RouteNode* next;  /* next neighbour, or NULL at the end of the list  */
} RouteNode;

/* One building on the map. */
typedef struct LocationVertex
{
    int id;                  /* the ID used in locations.txt and routes.txt */
    char name[MAX_NAME_LEN];
    RouteNode* routes;       /* first box of this building's neighbour list */
} LocationVertex;

/* The whole map: a fixed array of buildings and how many are filled in. */
typedef struct LocationGraph
{
    LocationVertex vertices[MAX_LOCATIONS];
    int count;
} LocationGraph;

/* Start with an empty map. */
void lg_init(LocationGraph* graph);

/* Give back the memory of every neighbour list. Calling it twice does no harm. */
void lg_free(LocationGraph* graph);

/* Add a building with its ID and name. */
LocationGraphStatus lg_add_location(LocationGraph* graph, int id, const char* name);

/* Add a two-way path between buildings idA and idB (it is stored in both lists). */
LocationGraphStatus lg_add_route(LocationGraph* graph, int idA, int idB, int walkingMinutes);

/*
 * Where is this building stored? Both return its index in the array,
 * or -1 if it is not on the map. The name search ignores capital letters,
 * so "library" finds "Library".
 */
int lg_find_index_by_id(const LocationGraph* graph, int id);
int lg_find_index_by_name(const LocationGraph* graph, const char* name);

/* How many buildings, and how many two-way paths, the map has. */
int lg_vertex_count(const LocationGraph* graph);
int lg_route_count(const LocationGraph* graph);

/*
 * Details of the building at a given index. For a wrong index,
 * lg_get_id returns NO_LOCATION and lg_get_name returns "Unknown location".
 */
int lg_get_id(const LocationGraph* graph, int index);
const char* lg_get_name(const LocationGraph* graph, int index);

/* The name of a building from its ID ("Unknown location" if the ID is not used). */
const char* lg_name_of(const LocationGraph* graph, int id);

/*
 * First neighbour of the building at index (NULL if it has none).
 * Follow ->next to visit the others. This is how Dijkstra walks the map.
 */
const RouteNode* lg_first_route(const LocationGraph* graph, int index);

/* Print every building with its neighbours and walking times. */
void showGraph(const LocationGraph* graph);

/*
 * Reading the map from files.
 *   locations.txt :  ID|Name                    e.g.  2|Library
 *   routes.txt    :  FromID|ToID|WalkingMinutes e.g.  1|2|5
 *
 * Empty lines and lines starting with '#' are skipped.
 * A wrong line is reported with its line number and skipped; the rest
 * of the file is still read.
 *
 * Both return how many records were added, or -1 if the file could not
 * be opened. *errorCount tells how many lines were rejected.
 * Load the locations first, because a route can only join known buildings.
 */
int lg_load_locations(LocationGraph* graph, const char* path, int* errorCount);
int lg_load_routes(LocationGraph* graph, const char* path, int* errorCount);

/* Turns a status code into a short message we can print. */
const char* lg_status_message(LocationGraphStatus status);

/*
 * Coming in Phase 3: removing a path, giving a new building the next free
 * ID, and saving the map back to locations.txt and routes.txt.
 */

#endif /* LOCATION_GRAPH_H */
