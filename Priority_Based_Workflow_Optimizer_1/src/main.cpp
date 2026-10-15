/*
  The only entry point of the program.
  It checks whether the console can creates the application, loads the data, runs the menu, 
  and lets the destructor release all memory.
  Compile using this command : 
  g++ -std=c++14 -Wall -Iinclude\cpp src\cpp\task.cpp src\cpp\heap.cpp src\cpp\task_manager.cpp src\cpp\schedule_manager.cpp src\cpp\task_graph.cpp src\cpp\location_graph.cpp src\cpp\dijkstra.cpp src\cpp\workflow_app.cpp src\main.cpp -o priority_workflow_optimizer.exe
*/
#include "workflow_app.h"
#include <iostream>

int main(){
    std::cout << "                    Priority Based Workflow Optimizer for Efficient Time Utilization\n\n";
    WorkflowApp app;  // Create the application (constructor prepares empty data structures).

    app.initialize();  // Load and validate the sample data. Problems are reported, not fatal.
    app.run();  // Run the console menu until the user chooses Exit.

    return 0;
}
