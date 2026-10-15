#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "task.h"             //max locations 
#include "location_graph.h"

enum DijkstraStatus {
    DIJKSTRA_OK = 0,
    DIJKSTRA_UNREACHABLE,        //no route connects source and destination
    DIJKSTRA_INVALID_LOCATION    //source or destination ID does not exist
};

struct ShortestPath {                  //Result of one shortest-path query.
    int totalMinutes;                  //minimum walking time
    int locationIds[MAX_LOCATIONS];    //source--destination
    int length;                        //number of IDs in locationIds
};

class DijkstraSolver {
public:
    explicit DijkstraSolver(const LocationGraph* campusGraph);

    DijkstraStatus findPath(int sourceId, int destinationId, ShortestPath& result) const;    //full answer: total time and the route itself.

    bool shortestTime(int sourceId, int destinationId, int& minutes) const;    //convenience: only the walking time. Returns false if unreachable/invalid.

    void printPath(const ShortestPath& path) const;   //e.g. it prints "Main Block -> Canteen -> Library (5 min)".

    static const char* statusMessage(DijkstraStatus status);

private:
    const LocationGraph* graph;
};

#endif