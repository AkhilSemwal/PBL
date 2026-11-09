#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "scheduler.h"
class GreedyScheduler : public Scheduler {
public:
    const char* name() const override;
protected:
    bool planSlots(const SchedulingInput& input, ScheduleResult& result) const override;
};
#endif
