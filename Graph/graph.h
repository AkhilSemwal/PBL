#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>

struct Edge
{
    int destination;
    int time;
};

class Graph
{
private:
    std::vector<std::vector<Edge>> graph;

public:
    Graph(int numberOfLocations);

    void addPath(int from, int to, int time);

    void showGraph();
};

#endif