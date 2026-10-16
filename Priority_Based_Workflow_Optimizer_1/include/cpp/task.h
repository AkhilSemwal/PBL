#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

#define MAX_TASKS 100
#define MAX_LOCATIONS 100
#define MAX_NAME_LEN 64

struct Task
{
    int id = 0;
    std::string name;
    int priority = 0;
    union
    {
        int durationMinutes = 0;
        int duration;
    };
    int locationId = 0;
    std::vector<int> prerequisites;
    bool isCompleted = false;
    int scheduledSlotId = -1;
};

#endif