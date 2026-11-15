# File Dependencies — Final (Phase 3)

This document shows which file needs which other file in the final Phase 3 code, why, and in which order the files can be built and tested. For the overall design see [architecture.md](architecture.md).

---

## 1. What is a "dependency"?

A file **depends on** another file when it uses something written there.

Real-life example: you cannot make tea without milk and tea leaves. Tea "depends on" milk and tea leaves. In the same way, `dijkstra.cpp` cannot work without the campus map from `location_graph.h` and the heap from `heap.h`.

In C++ a dependency is written as an `#include` line at the top of a file:

```cpp
#include "location_graph.h"   // "I need the campus map functions"
#include "heap.h"             // "I need the Heap class"
```

**Why this matters:** a file only compiles if every header it includes is present, and a change to a header can break every file that includes it.

---

## 2. The picture

Read an arrow `A --> B` as "A needs B".

```text
                                  main.cpp
                                     |
                                     v
                               workflow_app  (menus, integration)
                                     |
      +--------------+---------------+---------------+----------------+-------------------+
      v              v               v               v                v                   v
 task_manager  schedule_manager  task_graph   location_graph      dijkstra      optimizer / knapsack
                  |                  |                              |  |          (Greedy / 0/1 DP)
                  +--> heap          +--> heap                      |  +--> heap          |
                     (heapSort)         (MAX_HEAP, Kahn)            |     (MIN_HEAP)       v
                                                                    +--> location_graph  scheduler
                                                                                         |   |   |
                                                       dijkstra  <-----------------------+   |   |
                                                       task_graph  <--------------------------+   |
                                                       location_graph (types only)  <------------+

 task_manager and schedule_manager --> location_graph   (only to print building names)
 every module                      --> task             (shared types, limits, text helpers)
```

There are **no circular dependencies**: no file needs a file that, in turn, needs it back. Every header has an include guard (`#ifndef ... #define ... #endif`), so including it twice is harmless.

`optimizer` and `knapsack` do **not** depend on each other. Both depend only on `scheduler.h`, which provides the shared base class.

---

## 3. File-by-file table

| File | Owner | Includes (project headers) | What it uses from them |
|---|---|---|---|
| `task.h/.cpp` | Akhil | none | — (the base of everything: `TaskData`, `TimeSlot`, limits, `pu_` text helpers, `Task` class) |
| `heap.h/.cpp` | Akhil | none | — (`Heap` class and `heapSort()`) |
| `location_graph.h/.cpp` | Samriddhi | `task.h` | `MAX_LOCATIONS`, `MAX_NAME_LEN`, `NO_LOCATION`; text helpers `pu_read_line()`, `pu_split()`, `pu_parse_int()`, `pu_copy_text()`, `pu_equals_ignore_case()`, `pu_is_skippable()` |
| `task_graph.h/.cpp` | Samriddhi | `task.h`; `.cpp` also `heap.h` | `MAX_TASKS`; **`Heap(MAX_HEAP)`** for Kahn's topological sort |
| `task_manager.h/.cpp` | Piyush | `task.h`, `location_graph.h` | `TaskData`, text helpers; `lg_name_of()` to print a building's name |
| `schedule_manager.h/.cpp` | Piyush | `task.h`, `location_graph.h`; `.cpp` also `heap.h` | `TimeSlot`, text helpers, `pu_parse_time()`, `pu_format_time()`; `lg_name_of()`; **`heapSort()`** to sort periods |
| `dijkstra.h/.cpp` | Priyanshu | `task.h`, `location_graph.h`; `.cpp` also `heap.h` | `MAX_LOCATIONS`; `lg_find_index_by_id()`, `lg_vertex_count()`, `lg_first_route()`, `lg_get_id()`, `lg_name_of()`; **`Heap(MIN_HEAP)`** |
| `scheduler.h/.cpp` | Priyanshu | `task.h`, `task_graph.h`, `location_graph.h`; `.cpp` also `dijkstra.h` | `TaskData`, `TimeSlot`, `Task`; `tg_find_cycle()`, `tg_prerequisites_done()`; **`DijkstraSolver::shortestTime()`** for walking times |
| `optimizer.h/.cpp` | Priyanshu | `scheduler.h` | `Scheduler`, `SchedulingInput`, `ScheduleResult`, the shared helpers, `Task::operator>` |
| `knapsack.h/.cpp` | Piyush | `scheduler.h` | `Scheduler`, `SchedulingInput`, `ScheduleResult`, the shared helpers, `Task::operator>` |
| `workflow_app.h/.cpp` | Akhil | all module headers above; `.cpp` also `dijkstra.h` | Every module: loaders and savers, displays, editing functions, `tg_find_cycle()`, `tg_topological_sort()`, `showGraph()`, `DijkstraSolver` (route menu and itinerary), both planners |
| `main.cpp` | Akhil | `workflow_app.h` | `WorkflowApp` |

---

## 4. Who needs whom (team view)

| If this person changes... | ...these people must check their code still compiles |
|---|---|
| Akhil changes `task.h` | **Everyone** (it is shared by all) |
| Akhil changes `heap.h` | Piyush (`schedule_manager`), Samriddhi (`task_graph`), Priyanshu (`dijkstra`) |
| Samriddhi changes `location_graph.h` | Piyush (`task_manager`, `schedule_manager`, and through `scheduler.h` also `knapsack`), Priyanshu (`dijkstra`, `scheduler`, `optimizer`), Akhil (`workflow_app`) |
| Samriddhi changes `task_graph.h` | Priyanshu (`scheduler`, and through it `optimizer`), Piyush (`knapsack`), Akhil (`workflow_app`) |
| Piyush changes `task_manager.h` or `schedule_manager.h` | Akhil (`workflow_app`) |
| Priyanshu changes `dijkstra.h` | Priyanshu (`scheduler`), Akhil (`workflow_app`) |
| Priyanshu changes `scheduler.h` | Priyanshu (`optimizer`), Piyush (`knapsack`), Akhil (`workflow_app`) |
| Priyanshu changes `optimizer.h` or Piyush changes `knapsack.h` | Akhil (`workflow_app`) |

**Rule:** changing the inside of a `.cpp` file is safe for others. Changing a function's name or parameters in a `.h` file breaks everyone who uses it, so tell the team first.

---

## 5. Build order

Each level needs only the levels above it.

| Level | Files | Owner |
|---|---|---|
| 0 | `task.h/.cpp`, `heap.h/.cpp` | Akhil |
| 1 | `location_graph.h/.cpp`, `task_graph.h/.cpp`, `data/locations.txt`, `data/routes.txt` | Samriddhi |
| 2 | `task_manager.h/.cpp`, `schedule_manager.h/.cpp`, `data/tasks.txt`, `data/schedule.txt` | Piyush |
| 2 | `dijkstra.h/.cpp` | Priyanshu |
| 3 | `scheduler.h/.cpp` | Priyanshu |
| 4 | `optimizer.h/.cpp` | Priyanshu |
| 4 | `knapsack.h/.cpp` | Piyush |
| 5 | `workflow_app.h/.cpp`, `main.cpp`, `README.md`, `docs/` | Akhil |

**Test one file on its own.** The `-c` option compiles one file without building the whole program; it only needs the headers it includes. From the project folder:

```bash
g++ -std=c++17 -Wall -Iinclude/cpp -c src/cpp/scheduler.cpp -o scheduler.o
```

No output means the file compiles. Use `-std=c++14` with an older compiler, and delete the `.o` file afterwards.

**Build the whole program** with the one-line command in the [README](../README.md#10-how-to-run-the-project).

---

## 6. What changed from the Phase 2 plan

| Phase 2 plan | Final Phase 3 code |
|---|---|
| The plan types (`ScheduleResult`, `Scheduler`, `SchedulingInput`) were to be added to `task.h` | They are in a separate `scheduler.h/.cpp`, because they need `task_graph.h` and `location_graph.h`, and `task.h` must stay the lowest header that every module includes |
| `knapsack` was to depend on `optimizer` through a shared rules class | Both planners depend only on `scheduler.h` and inherit the abstract `Scheduler` class; the shared rules are in `Scheduler::plan()` and its helpers |
| `task_graph` needed only `task.h` | `task_graph.cpp` also includes `heap.h`, for the max-heap used by Kahn's algorithm |
| `workflow_app` used the Phase 2 modules only | It also uses `scheduler`, `optimizer` and `knapsack`, and `dijkstra` for the itinerary |
