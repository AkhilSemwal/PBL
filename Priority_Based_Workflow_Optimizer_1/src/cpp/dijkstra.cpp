#include "dijkstra.h"
#include "heap.h"

#include <climits>
#include <iostream>

// Distance value meaning "not reached yet".
static const int INFINITE_DISTANCE = INT_MAX;

DijkstraSolver::DijkstraSolver(const LocationGraph* campusGraph) : graph(campusGraph){}

DijkstraStatus DijkstraSolver::findPath(int sourceId, int destinationId, ShortestPath& result) const
{
    result.totalMinutes = 0;
    result.length = 0;

    if (graph == nullptr) {
        return DIJKSTRA_INVALID_LOCATION;
    }
    int source = lg_find_index_by_id(graph, sourceId);
    int destination = lg_find_index_by_id(graph, destinationId);
    if (source == -1 || destination == -1) {
        return DIJKSTRA_INVALID_LOCATION;
    }

    /* Special case: already there. */
    if (source == destination) {
        result.locationIds[0] = sourceId;
        result.length = 1;
        return DIJKSTRA_OK;
    }

    int vertexCount = lg_vertex_count(graph);

    // Step 1: allocate the work arrays.
    int* distance = new int[vertexCount];
    int* previous = new int[vertexCount];
    bool* finalised = new bool[vertexCount];
    Heap frontier(MIN_HEAP, vertexCount);   // key = distance, value = vertex index

    // Step 2: every distance starts at infinity, the source at zero.
    for (int index = 0; index < vertexCount; index++) {
        distance[index] = INFINITE_DISTANCE;
        previous[index] = -1;
        finalised[index] = false;
    }
    distance[source] = 0;
    frontier.push(0, source);

    DijkstraStatus status = DIJKSTRA_UNREACHABLE;

    // Step 3: repeatedly take the closest location that is not finalised yet.
    HeapNode closest;
    while (frontier.pop(closest)) {
        int current = closest.value;

        // Skip stale entries: already finalised, or an older, longer distance.
        if (finalised[current]) {
            continue;
        }
        if (closest.key > distance[current]) {
            continue;
        }

        finalised[current] = true;

        // The destination's distance can no longer improve: stop early.
        if (current == destination) {
            status = DIJKSTRA_OK;
            break;
        }

        // Step 4: relax every route leaving the current location.
        const RouteNode* route = lg_first_route(graph, current);
        while (route != nullptr) {
            int neighbour = route->toIndex;
            //Guard against integer overflow with absurdly large weights.
            bool safeToAdd = route->walkingMinutes <= INFINITE_DISTANCE - 1 - distance[current];
            if (!finalised[neighbour] && safeToAdd) {
                int candidate = distance[current] + route->walkingMinutes;
                if (candidate < distance[neighbour]) {
                    distance[neighbour] = candidate;
                    previous[neighbour] = current;
                    frontier.push(candidate, neighbour);
                }
            }
            route = route->next;
        }
    }

    // Step 5: rebuild the route by following "previous" links backwards.
    if (status == DIJKSTRA_OK) {
        int reversed[MAX_LOCATIONS];
        int count = 0;
        int step = destination;
        while (step != -1 && count < MAX_LOCATIONS) {
            reversed[count] = step;
            count++;
            step = previous[step];
        }
        for (int position = 0; position < count; position++) {
            result.locationIds[position] = lg_get_id(graph, reversed[count - 1 - position]);
        }
        result.length = count;
        result.totalMinutes = distance[destination];
    }

    delete[] distance;
    delete[] previous;
    delete[] finalised;
    return status;
}

bool DijkstraSolver::shortestTime(int sourceId, int destinationId, int& minutes) const
{
    ShortestPath path;
    if (findPath(sourceId, destinationId, path) != DIJKSTRA_OK) {
        return false;
    }
    minutes = path.totalMinutes;
    return true;
}

void DijkstraSolver::printPath(const ShortestPath& path) const
{
    for (int position = 0; position < path.length; position++) {
        std::cout << lg_name_of(graph, path.locationIds[position]);
        if (position + 1 < path.length) {
            std::cout << " -> ";
        }
    }
    std::cout << "  (total walking time: " << path.totalMinutes << " min)\n";
}

const char* DijkstraSolver::statusMessage(DijkstraStatus status)
{
    switch (status) {
    case DIJKSTRA_OK:
        return "OK";
    case DIJKSTRA_UNREACHABLE:
        return "destination cannot be reached from the source";
    case DIJKSTRA_INVALID_LOCATION:
        return "unknown source or destination location";
    }
    return "unknown status";
}
