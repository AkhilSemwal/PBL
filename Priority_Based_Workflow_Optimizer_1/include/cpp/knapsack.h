/*
 * knapsack.h
 * Owner: Piyush (0/1 Knapsack DP planner)
 *
 * KnapsackScheduler treats every free slot as a knapsack:
 *   capacity = minutes in the slot
 *   item     = one ready task, taken whole or not at all (0/1)
 *   weight   = duration of the task
 *   value    = priority (ties: the set that uses more minutes wins)
 *
 * Dynamic programming finds the best SET of tasks for the slot. The chosen
 * tasks are then put in walking order (nearest building next) and the real
 * route is timed, including walking and the walk to the next class. Walking
 * depends on the order, so it cannot be a fixed weight: if the real route is
 * too long by k minutes, the capacity is made k minutes smaller and the DP
 * is solved again, until the chosen set really fits.
 *
 * Limits: each slot is optimal on its own, not the whole day at once, and
 * a task can only use prerequisites that were planned in EARLIER slots.
 */
#ifndef KNAPSACK_H
#define KNAPSACK_H

#include "scheduler.h"

class KnapsackScheduler : public Scheduler {
public:
    const char* name() const override;

protected:
    bool planSlots(const SchedulingInput& input, ScheduleResult& result) const override;
};

#endif /* KNAPSACK_H */
