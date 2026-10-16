# File Dependencies — Phase 2

## 1. What is a "dependency"?

A file **depends on** another file when it uses something written there.

Real-life example: you cannot make tea without milk and tea leaves. Tea "depends on" milk and tea leaves. In the same way, `dijkstra.cpp` cannot work without the campus map from `location_graph.h` and the heap from `heap.h`.

In C++ a dependency is written as an `#include` line at the top of a file:

```cpp
#include "location_graph.h"   // "I need the campus map functions"
#include "heap.h"             // "I need the Heap class"
```

**Why this matters for the team:** a file only compiles if every header it includes is present. Commit files in the order shown in section 5.

---

## 2. The picture

Read an arrow `A ──> B` as "A needs B".

```
                              main.cpp
                                 │
                                 ▼
                     workflow_app  (Akhil: the menu)
   ┌──────────────┬──────────────┼───────────────┬──────────────┐
   ▼              ▼              ▼               ▼              ▼
task_manager  schedule_manager  task_graph   location_graph   dijkstra
 (Piyush)      (Piyush)        (Samriddhi)    (Samriddhi)    (Priyanshu)
   │              │   │            │               │           │    │
   │              │   └──> heap    │               │           │    └──> heap
   │              │       (heapSort)               │           │       (MIN_HEAP)
   │              │                │               │           │
   ├──> location_graph (only to print place names) │           └──> location_graph
   │              ├──> location_graph              │                 (the map it searches)
   ▼              ▼                ▼               ▼
 ────────────────────────────  task  (Akhil)  ────────────────────────────
           shared data types and text helpers, used by everyone
```

There are **no circular dependencies**: no file needs a file that, in turn, needs it back. Every header also has an include guard (`#ifndef ... #define ... #endif`), so including it twice is harmless.

---

## 3. File-by-file table

| File | Owner | Includes (needs) | What it uses from them |
|---|---|---|---|
| `task.h/.cpp` | Akhil | nothing from the project | — (the base of everything) |
| `heap.h/.cpp` | Akhil | nothing from the project | — |
| `location_graph.h/.cpp` | Samriddhi | `task.h` | `MAX_LOCATIONS`, `MAX_NAME_LEN`, text helpers (`pu_read_line`, `pu_split`, `pu_parse_int`) |
| `task_graph.h/.cpp` | Samriddhi | `task.h` | `MAX_TASKS` |
| `task_manager.h/.cpp` | Piyush | `task.h`, `location_graph.h` | `TaskData`, text helpers; `lg_name_of()` to print a location's name |
| `schedule_manager.h/.cpp` | Piyush | `task.h`, `location_graph.h`, `heap.h` | `TimeSlot`, text helpers, `pu_parse_time()`; `lg_name_of()`; **`heapSort()`** to sort periods |
| `dijkstra.h/.cpp` | Priyanshu | `task.h`, `location_graph.h`, `heap.h` | `MAX_LOCATIONS`; `lg_find_index_by_id()`, `lg_vertex_count()`, `lg_first_route()`, `lg_get_id()`, `lg_name_of()`; **`Heap(MIN_HEAP)`**, `push()`, `pop()` |
| `workflow_app.h/.cpp` | Akhil | all of the above | Calls every module: loaders, displays, `tg_find_cycle()`, `showGraph()`, `DijkstraSolver` |
| `main.cpp` | Akhil | `workflow_app.h` | `WorkflowApp` |

---

## 4. Who needs whom (team view)

| If this person changes... | ...these people must check their code still compiles |
|---|---|
| Akhil changes `task.h` | **Everyone** (it is shared by all) |
| Akhil changes `heap.h` | Piyush (`schedule_manager`), Priyanshu (`dijkstra`) |
| Samriddhi changes `location_graph.h` | Piyush (`task_manager`, `schedule_manager`), Priyanshu (`dijkstra`), Akhil (`workflow_app`) |
| Samriddhi changes `task_graph.h` | Akhil (`workflow_app`) |
| Piyush changes `task_manager.h` or `schedule_manager.h` | Akhil (`workflow_app`) |
| Priyanshu changes `dijkstra.h` | Akhil (`workflow_app`) |

**Rule:** changing the inside of a `.cpp` file is safe for others. Changing a function's name or parameters in a `.h` file breaks everyone who uses it, so tell the team first.

---

## 5. Build and commit order

Commit (and test) in this order. Each level needs only the levels above it.

| Level | Files | Owner |
|---|---|---|
| 0 | `task.h/.cpp`, `heap.h/.cpp` | Akhil |
| 1 | `location_graph.h/.cpp`, `task_graph.h/.cpp`, `data/locations.txt`, `data/routes.txt` | Samriddhi |
| 2 | `task_manager.h/.cpp`, `schedule_manager.h/.cpp`, `data/tasks.txt`, `data/schedule.txt` | Piyush |
| 2 | `dijkstra.h/.cpp` | Priyanshu |
| 3 | `workflow_app.h/.cpp`, `main.cpp`, `Makefile`, `README.md`, `docs/` | Akhil |

**Test your own file before everyone has finished.** The `-c` option compiles one file without building the whole program. It only needs the headers to exist. From the project folder:

```bash
g++ -std=c++14 -Wall -Iinclude/cpp -c src/cpp/dijkstra.cpp -o dijkstra.o
```

No output means your file compiles. Delete the `.o` file afterwards.

---

## 6. Dependencies coming in Phase 3

| New file (Phase 3) | Owner | Will need |
|---|---|---|
| `optimizer.h/.cpp` (Greedy scheduler) | Priyanshu | `task.h` (new: `ScheduleResult`, `Scheduler`, `SchedulingInput`), `dijkstra.h` (walking times), `task_graph.h` (new: `tg_prerequisites_done()`), `location_graph.h` (`lg_name_of()`) |
| `knapsack.h/.cpp` (0/1 Knapsack DP) | Piyush | `task.h`, `optimizer.h` (the shared rules class `SchedulingContext`) |
| `workflow_app.h/.cpp` (extended) | Akhil | `optimizer.h`, `knapsack.h` for the planning, comparison and itinerary options |

So in Phase 3 the order becomes: Akhil adds the new types to `task.h` → Samriddhi adds `tg_prerequisites_done()` → Priyanshu adds `optimizer` → Piyush adds `knapsack` → Akhil connects them in the menu.

```
                     workflow_app
                 ┌────────┴─────────┐
                 ▼                  ▼
            knapsack ──────────> optimizer
            (Piyush)            (Priyanshu)
                                 │   │    │
                      ┌──────────┘   │    └──────────┐
                      ▼              ▼               ▼
                  dijkstra       task_graph        task
```
