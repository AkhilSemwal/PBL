#include "graph.h"

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
    cout << "\n";
    cout << "==============================\n";
    cout << "       LOCATION GRAPH\n";
    cout << "==============================\n";

    cout << "\nA = Department";
    cout << "\nB = Library";
    cout << "\nC = Canteen";
    cout << "\nD = Lab\n";

    cout << "\nPaths:\n";

    cout << "A -> B = 5 minutes\n";
    cout << "A -> C = 7 minutes\n";
    cout << "B -> D = 4 minutes\n";
    cout << "C -> D = 6 minutes\n";

    cout << "\nGraph using adjacency list:\n";

    for (int i = 0; i < graph.size(); i++)
    {
        char location = 'A' + i;

        cout << location << " -> ";

        for (int j = 0; j < graph[i].size(); j++)
        {
            char destination = 'A' + graph[i][j].destination;

            cout << destination
                 << " (" << graph[i][j].time << " min)";

            if (j < graph[i].size() - 1)
            {
                cout << ", ";
            }
        }

        cout << endl;
    }
}