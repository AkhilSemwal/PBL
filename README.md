# Priority Based Workflow Optimizer for Efficient Time Utilization

A C++ console application that decides which pending tasks a student should do in the free periods of a college day. It selects tasks using their **priority**, **duration**, **prerequisites**, the **remaining capacity** of each free slot, and the **walking time** between campus locations.

Built in **C++17**. The binary heap / priority queue is implemented from scratch without STL.

---

## 1. Features

- Task dependency graph with **DFS cycle detection**
- **Topological ordering** of tasks using Kahn's algorithm with a priority queue
- Free slot extraction from the daily timetable (classes and labs are never used)
- **Dijkstra** shortest walking time between campus locations
- Usable time per slot after subtracting walking time
- **Greedy** priority-based scheduler
- **0/1 Knapsack DP** scheduler for optimal task selection per slot
- Greedy vs DP comparison on the same input
- Report with scheduled tasks, deferred tasks, time utilization and achieved priority
- Interactive console menu

---

## 2. Team

| Member | Responsibility |
|---|---|
| Akhil Semwal (Lead) | Integration, common data model, heap & heap sort, console app, build system |
| Priyanshu Gairola | Greedy workflow optimizer, Dijkstra shortest walking time |
| Samriddhi | Task dependency graph, DFS cycle detection, topological sort, campus location graph |
| Piyush | Task manager, schedule manager, 0/1 knapsack DP, greedy vs DP comparison |

---

## 3. Project Structure

```text
Priority_Based_Workflow_Optimizer/
├── Makefile
├── README.md
├── include/cpp/
│   ├── task.h               # Task and TimeSlot definitions (shared by all modules)
│   ├── heap.h               # Binary heap / priority queue + heap sort (no STL)
│   ├── task_manager.h       # Add, find and list tasks
│   ├── schedule_manager.h   # Manage time slots and capacity
│   ├── task_graph.h         # Task dependencies, DFS cycle detection, topological sort
│   ├── location_graph.h     # Campus locations and walking paths
│   ├── optimizer.h          # Greedy priority-based scheduler
│   ├── dijkstra.h           # Shortest walking time between locations
│   ├── knapsack.h           # 0/1 knapsack DP scheduler
│   └── workflow_app.h       # Interactive console menu
├── src/
│   ├── main.cpp
│   └── cpp/                 # One .cpp file for each header above
├── data/
│   ├── tasks.txt            # Pending tasks: id, name, priority, duration, venue, prerequisites
│   ├── schedule.txt         # Daily timetable (classes and free periods)
│   ├── locations.txt        # Campus venues
│   └── routes.txt           # Walking paths with time in minutes
└── docs/
```

---

## 4. How It Works

```text
Tasks + Timetable + Campus Map
            │
            ▼
1. Dependency check        DFS cycle detection, topological order
            │
            ▼
2. Free slot extraction    only free periods, classes and labs skipped
            │
            ▼
3. Usable time             Dijkstra walking time subtracted from each slot
            │
            ▼
4. Task selection          greedy or 0/1 knapsack DP per slot
            │
            ▼
5. Report                  scheduled, deferred, utilization, achieved priority
```

### Step 1: Dependency check

Tasks and their prerequisites form a directed graph (adjacency list). DFS with three states (unvisited, visiting, done) detects circular dependencies, which are reported to the user. Kahn's algorithm then produces a valid order; a max-heap is used instead of a normal queue so that, among tasks that are ready, the highest priority one comes first.

### Step 2: Free slot extraction

Only periods marked as free in the timetable are given capacity. Class and lab periods are never assigned tasks.

### Step 3: Usable time

Campus locations form a weighted undirected graph. Dijkstra (with a min-heap) gives the shortest walking time between any two venues. For a task at venue `v`, between a commitment at `u` and the next at `w`:

```
Usable Time = Free Slot Duration − walk(u → v) − walk(v → w)
```

### Step 4: Task selection

**Greedy:** for each slot, repeatedly pick the highest-priority task whose prerequisites are done and which fits in the remaining time.

**0/1 Knapsack DP:** for each slot, capacity = usable minutes, weight = task duration, value = task priority. DP returns the subset with maximum total priority.

Greedy is fast but not always optimal. Example with a 55-minute slot:

| Approach | Tasks chosen | Total priority |
|---|---|---|
| Greedy | One 55-min task (priority 9) | 9 |
| Knapsack DP | Two 25-min tasks (priority 6 + 5) | 11 |

The application runs both on the same input and compares time utilization and achieved priority.

### Step 5: Report

- Total, scheduled and deferred tasks
- Available, used and unused time
- Time utilization (%)
- Total and achieved priority (%)
- List of deferred tasks with priority and duration

---

## 5. Data Structures & Complexity

| Component | Data structure / algorithm | Time complexity |
|---|---|---|
| Heap push / pop | Binary heap (array-based) | O(log n) |
| Heap top | Binary heap | O(1) |
| Heap sort | In-place heap sort | O(n log n), O(1) extra space |
| Cycle detection | DFS on adjacency list | O(V + E) |
| Topological sort | Kahn's algorithm + max-heap | O(V log V + E) |
| Shortest walking time | Dijkstra with min-heap (lazy deletion) | O(E log E) |
| Greedy scheduler | Repeated selection per slot | O(S · n² log n) |
| Knapsack scheduler | 0/1 knapsack DP | O(n · T) per slot |

*n = tasks, S = slots, V / E = graph vertices / edges, T = slot capacity in minutes*

---

## 6. Build & Run

**Requirements:** `g++` with C++17 support and `make`.

```bash
make        # build
make run    # run the console application
```

---

## 7. Team Workflow

1. Every member works on their own branch.
2. Pull requests are reviewed and merged by the team lead.
3. Each module is checked with sample inputs before merging.
