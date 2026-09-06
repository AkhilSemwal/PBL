#include "graph.h"
#include <iostream>

Graph::Graph(int numberOfLocations)
{
    graph.resize(numberOfLocations);
}

void Graph::addPath(int from, int to, int time)
{
    Edge newEdge;

    newEdge.destination = to;
    newEdge.time = time;

    graph[from].push_back(newEdge);
}

void Graph::showGraph()
{
    std::cout << "==============================" << std::endl;
    std::cout << "       LOCATION GRAPH" << std::endl;
    std::cout << "==============================" << std::endl;

    std::cout << std::endl;

    std::cout << "A = Department" << std::endl;
    std::cout << "B = Library" << std::endl;
    std::cout << "C = Canteen" << std::endl;
    std::cout << "D = Lab" << std::endl;

    std::cout << std::endl;

    std::cout << "Paths:" << std::endl;

    std::cout << "A -> B = 5 minutes" << std::endl;
    std::cout << "A -> C = 7 minutes" << std::endl;
    std::cout << "B -> D = 4 minutes" << std::endl;
    std::cout << "C -> D = 6 minutes" << std::endl;

    std::cout << std::endl;

    std::cout << "Graph using adjacency list:" << std::endl;

    for(int i = 0; i < graph.size(); i++)
    {
        char location = 'A' + i;

        std::cout << location << " -> ";

        for(int j = 0; j < graph[i].size(); j++)
        {
            char destination = 'A' + graph[i][j].destination;

            std::cout << destination;
            std::cout << " (" << graph[i][j].time << " min)";

            if(j < graph[i].size() - 1)
            {
                std::cout << ", ";
            }
        }

        std::cout << std::endl;
    }
}