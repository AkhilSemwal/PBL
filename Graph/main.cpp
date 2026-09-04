#include "graph.h"

int main()
{
    Graph campus(4);

    
    campus.addPath(0, 1, 5);
    campus.addPath(0, 2, 7);
    campus.addPath(1, 3, 4);
    campus.addPath(2, 3, 6);

    campus.showGraph();

    return 0;
}