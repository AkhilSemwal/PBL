/*
 * The only entry point of the program. It creates the application, loads
 * the data, runs the menu, and lets the destructor release all memory.
 */
#include "workflow_app.h"

#include <iostream>

int main()
{
    std::cout << "Priority Based Workflow Optimizer for Efficient Time Utilization\n\n";

    // Create the application (constructor prepares empty data structures).
    WorkflowApp app;

    // Load and validate the data files. Problems are reported, not fatal.
    app.initialize();

    // Run the console menu until the user chooses Exit.
    app.run();

    // Clean-up happens automatically in ~WorkflowApp() when "app" goes out of scope.
    return 0;
}
