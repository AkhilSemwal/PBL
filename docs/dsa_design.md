# Data Structures and Algorithms (DSA) Design

This document describes every data structure and algorithm implemented in the Priority Based Workflow Optimizer, why it was chosen, and its complexity. All of them are written by hand — the project does not use STL containers (`vector`, `map`, ...) or STL algorithms (`sort`, `priority_queue`, ...).

For how the modules fit together, see [architecture.md](architecture.md).

---

## 1. Summary

| Data structure / algorithm | File | Used for |
|---|---|---|
| Arrays of structures | `task.h`, `schedule_manager.h`, `scheduler.h` | Tasks, periods, free slots, plan results |
| Dynamic arrays | `task_manager.cpp`, `heap.cpp`, `dijkstra.cpp`, `knapsack.cpp`, `workflow_app.cpp` | Task list, heap storage, Dijkstra arrays, DP table, `Task` objects |
| Singly linked lists | `location_graph.cpp`, `task_graph.cpp` | Neighbour lists of both graphs |
| Binary heap (min and max) | `heap.h/.cpp` | Dijkstra (min-heap), Kahn's algorithm (max-heap) |
| Weighted undirected graph | `location_graph.h/.cpp` | Campus buildings and walking paths |
| Directed graph | `task_graph.h/.cpp` | Task prerequisites |
| Heap sort | `heap.cpp` | Sorting timetable periods by start time |
| Insertion sort | `workflow_app.cpp` | Pending tasks by priority |
| Linear search | all managers and graphs | Finding a task, period or building by ID or name |
| Interval overlap check | `schedule_manager.cpp` | Rejecting clashing periods |
| DFS (three states) | `task_graph.cpp` | Detecting circular prerequisites |
| Dijkstra's algorithm | `dijkstra.cpp` | Shortest walking route and time |
| Kahn's topological sort | `task_graph.cpp` | A valid order of all tasks |
| Greedy algorithm | `optimizer.cpp` | Greedy planner |
| 0/1 Knapsack dynamic programming | `knapsack.cpp` | Knapsack planner |

---

## 2. Data structures

### 2.1 Arrays of structures (`task.h`, `schedule_manager.h`, `scheduler.h`)

- **Purpose:** store records whose maximum number is small and known in advance.
- **Example — one task:**
  ```cpp
  typedef struct TaskData {
      int id;                                   /* unique task ID, >= 1           */
      char name[MAX_NAME_LEN];                  /* human readable name            */
      int durationMinutes;                      /* how long the task takes        */
      int priority;                             /* 1..10, higher = more important */
      int locationId;                           /* where the task must be done    */
      int prerequisiteCount;                    /* number of used entries below   */
      int prerequisiteIds[MAX_PREREQUISITES];   /* tasks that must finish first   */
      int isCompleted;                          /* 1 = already done, 0 = pending  */
  } TaskData;
  ```
- **Where:** `ScheduleManager` keeps `Period periods[MAX_PERIODS]` and `TimeSlot freeSlots[MAX_SLOTS]`; `ScheduleResult` keeps fixed arrays of `PlannedTask`, `DeferredTask` and `SlotUsage`; both graphs keep a fixed array of vertices.
- **Limits:** `MAX_TASKS` = 100, `MAX_SLOTS` = `MAX_PERIODS` = 50, `MAX_LOCATIONS` = 50, `MAX_PREREQUISITES` = 5, names up to 63 characters.
- **Why:** direct access by index, no allocation. Because `ScheduleResult` contains only fixed arrays, a whole plan can be copied safely.
- **Complexity:** access O(1); adding at the end O(1); deleting from the middle O(n) (later items are shifted).

### 2.2 Dynamic arrays

| Where | Allocation | Size |
|---|---|---|
| `TaskManager::tasks` | `malloc` in `tm_init()`, `free` in `tm_free()` | `MAX_TASKS` records, allocated once |
| `Heap::data` | `new[]` in the constructor, `delete[]` in the destructor | Starts at the given capacity and **doubles** when full (`grow()`) |
| Dijkstra `distance[]`, `previous[]`, `finalised[]` | `new[]` per query, `delete[]` at the end | Number of buildings |
| Knapsack DP table | `new (std::nothrow) int[(n + 1) * (W + 1)]` per solve | Tasks × slot minutes |
| `WorkflowApp::taskObjects` | `new[]` / `delete[]` | Number of tasks |

- **Why:** the size is known only at run time.
- **Complexity:** heap push is amortised O(1) for the array growth (each doubling copies all items, but doublings are rare).

### 2.3 Singly linked lists (`location_graph`, `task_graph`)

- **Purpose:** the neighbour list of each graph vertex. Each vertex has a different number of edges, so a list grows as needed.
- **Node structures:**
  ```cpp
  typedef struct RouteNode {          /* campus graph */
      int toIndex;                    /* position of the neighbour in the vertices array */
      int walkingMinutes;
      struct RouteNode* next;         /* next neighbour, or NULL at the end of the list  */
  } RouteNode;

  typedef struct DependencyNode {     /* prerequisite graph */
      int toIndex;
      struct DependencyNode* next;
  } DependencyNode;
  ```
- **Operations and complexity** (d = length of the list):

| Operation | How | Time |
|---|---|---|
| Append a node | Walk to the end and link it there (keeps the file order); `location_graph` uses a pointer-to-pointer (`RouteNode**`) | O(d) |
| Check for a duplicate edge | Walk the list | O(d) |
| Remove a route | `unlink_route()` walks with a `RouteNode**`, re-links around the node and frees it — done in **both** lists | O(d) |
| Free all nodes | Walk and `free()` each node | O(d) |

### 2.4 Binary heap / priority queue (`heap.h`, `heap.cpp`)

- **Purpose:** always give the smallest item (min-heap) or the largest item (max-heap) quickly.
- **Structures:**
  ```cpp
  typedef struct HeapNode {
      int key;      /* what is compared: a distance or a priority */
      int value;    /* what is carried: a vertex index            */
  } HeapNode;

  typedef enum HeapType { MIN_HEAP, MAX_HEAP } HeapType;
  ```
  `class Heap` keeps a private dynamic array `data`, `count`, `capacity` and `type`. Copying a `Heap` is deleted, because two copies would free the same array.
- **Array layout:** for the node at index `i`, the parent is at `(i - 1) / 2` and the children are at `2i + 1` and `2i + 2`.
- **Order:** min-heap — smaller key first; max-heap — larger key first. **Equal keys: the smaller value comes first**, so results are always the same.
- **Operations:**

| Operation | How | Time |
|---|---|---|
| `push(key, value)` | Grow if full, place at the end, sift up | O(log n) (amortised for growth) |
| `pop(out)` | Return the root, move the last item to the root, sift down; `false` if empty | O(log n) |
| `top(out)` | Read the root without removing it | O(1) |
| `isEmpty()`, `size()`, `clear()` | — | O(1) |

- **No decrease-key:** Dijkstra uses **lazy deletion** instead — when a shorter distance is found, a new entry is pushed and the old one is skipped when it is popped.

### 2.5 Weighted undirected graph (`location_graph.h/.cpp`)

- **Purpose:** the campus map. Vertices are buildings; edges are footpaths weighted by walking minutes.
- **Representation:** `LocationGraph` holds `LocationVertex vertices[MAX_LOCATIONS]`; each vertex has an `id`, a `name` and the head of its `RouteNode` list (adjacency list).
- **Undirected:** a path `A–B` is stored as two nodes, `A → B` in A's list and `B → A` in B's list. `lg_route_count()` counts all nodes and divides by 2.
- **Rules enforced when adding a route:** walking time not negative (Dijkstra needs non-negative weights), the two buildings must be different and must exist, no duplicate route. Both nodes are allocated before either is linked, so a failed `malloc` never leaves half a path.
- **Lookups:** by ID, or by name ignoring capital letters (`"library"` finds `"Library"`).
- **Complexity:** space O(V + R); add a building O(V); add a route O(V + d); visit all neighbours of a vertex O(degree).

### 2.6 Directed graph (`task_graph.h/.cpp`)

- **Purpose:** task prerequisites.
- **Edge direction:** if task B needs task A, the edge is `A → B` ("A must be done before B"). A vertex's list therefore holds the tasks **waiting for it** (its dependents).
- **Representation:** `TaskGraph` holds `TaskVertex vertices[MAX_TASKS]`; each vertex has `taskId`, `priority` (used by Kahn's algorithm) and the head of its `DependencyNode` list.
- **Rules enforced:** no self-loop ("T3 needs T3"), both tasks must exist, no duplicate edge.
- **Reverse lookup:** `tg_get_prerequisites()` finds the tasks a given task needs by scanning every list — O(n + E), where n = tasks and E = edges.

### 2.7 DP table (`knapsack.cpp`)

- A two-dimensional table stored in one flat dynamic array: entry `best[i][w]` is at `best[i * (W + 1) + w]`.
- Size `(n + 1) × (W + 1)`, where n = ready tasks for the slot and W = the slot's minutes (or the reduced capacity).
- Allocated with `new (std::nothrow)`; if memory runs out the planner stops cleanly. Freed with `delete[]` after each solve.

---

## 3. Algorithms

### 3.1 Heap sort (`heapSort()` in `heap.cpp`)

- **Used for:** sorting timetable periods by start time (`sm_sort_periods()`), because the file may list periods in any order.
- **How:** build a heap inside the array, then repeatedly swap the root to the end and sift down the rest. Each period is sorted as a `HeapNode` (key = start minute, value = its position), then the periods are copied in the new order.
- **Complexity:** O(n log n) time, O(1) extra space (in place). Not stable, but periods never share a start time because overlapping periods are rejected.

### 3.2 Insertion sort (`showTasksByPriority()` in `workflow_app.cpp`)

- **Used for:** listing pending tasks from most to least important (menu 4 → 3).
- **How:** sorts an array of positions; each task moves left while it ranks higher than its neighbour (`Task::operator>`: higher priority, then shorter, then lower ID).
- **Complexity:** O(n²) worst case, O(n) when already in order; O(n) extra space. Simple, and n is at most 100.

### 3.3 Linear search

- **Used for:** `tm_find_index()`, `lg_find_index_by_id()`, `lg_find_index_by_name()`, `tg_find_index()`, duplicate-ID checks.
- **Complexity:** O(n). Fast enough for the fixed limits; no hash table is needed.

### 3.4 Interval overlap check (`sm_periods_overlap()`)

- Periods are half-open intervals `[start, end)`. Two periods clash when **each starts before the other ends**:
  ```text
  s1 < e2  and  s2 < e1
  ```
- 10:00–11:00 and 10:30–11:30 clash (600 < 690 and 630 < 660); 10:00–11:00 and 11:00–12:00 only touch (660 < 660 is false), so they are allowed.
- **Complexity:** O(1) per pair; adding a period checks it against every existing period, O(P).

### 3.5 Free-slot building (`sm_build_free_slots()`)

- One pass over the sorted periods. Every `FREE` period becomes a `TimeSlot`; the building of the class just before it is the start location, and the building of the class just after it is where the student must be when it ends (`NO_LOCATION` if the neighbouring period is also free).
- **Complexity:** O(P).

### 3.6 DFS cycle detection (`tg_find_cycle()`)

- **Problem:** if T1 needs T2 and T2 needs T1, neither can ever start.
- **How:** recursive DFS with three states per task: **unvisited**, **visiting** (on the current path), **done** (fully explored). Reaching a task that is still *visiting* means the search came back along its own path — a cycle. Meeting a *done* task is not a cycle. A `parent[]` array is used to print the cycle, for example `T1 -> T2 -> T1`.
- **Complexity:** O(n + E) time (n = tasks, E = prerequisite edges); O(n) extra space for `state[]`, `parent[]` and the recursion.

### 3.7 Dijkstra's shortest path (`DijkstraSolver::findPath()`)

- **Problem:** the shortest walking time between two buildings. The direct path is not always the shortest (Department Block → Computer Lab: 12 minutes directly, 9 via the Library).
- **How:**
  1. Convert building IDs to indexes; return `DIJKSTRA_INVALID_LOCATION` for an unknown ID, and 0 minutes if both are the same.
  2. `distance[]` = infinity (`INT_MAX`) except the source (0); `previous[]` = −1; `finalised[]` = false. Push the source into a **min-heap** (key = distance, value = index).
  3. Pop the closest building. Skip **stale** entries (already finalised, or key larger than the current distance) — lazy deletion. Finalise it; if it is the destination, stop early.
  4. **Relax** every neighbour: if `distance[current] + walk < distance[neighbour]`, update the distance and `previous[]`, and push the new entry. An overflow guard prevents integer overflow.
  5. If the heap empties first, the destination is **unreachable**. Otherwise rebuild the route by following `previous[]` back from the destination and reversing it.
- **Complexity:** O((V + R) log V) time; O(V + R) extra space (the heap can hold one entry per successful relaxation).
- **Requirement:** all weights non-negative — enforced by `location_graph`.

### 3.8 Kahn's topological sort (`tg_topological_sort()`)

- **Problem:** print an order of all tasks in which every task comes after its prerequisites (menu 4 → 6).
- **How:**
  1. Compute every task's **in-degree** (number of prerequisites) by walking all lists.
  2. Push every task with in-degree 0 into a **max-heap** with key = priority, so the most important ready task comes out first (equal priority: the task stored first).
  3. Pop a task, add it to the order, and decrease the in-degree of each of its dependents; a dependent whose in-degree becomes 0 is pushed.
  4. If fewer tasks were output than exist, the remaining tasks are in a cycle (`TG_ERR_CYCLE`).
- **Complexity:** O((n + E) log n) time; O(n) extra space.

### 3.9 Greedy planner (`GreedyScheduler::planSlots()`)

- **How:** for each free slot in time order, repeatedly take the best-ranked task (`Task::operator>`) that is pending, has its prerequisites done or planned, is reachable, and satisfies
  ```text
  used + walk in + duration + walk to next class  <=  slot length
  ```
  When nothing fits, add the walk to the next class and move on. Walking times come from Dijkstra.
- **Property:** fast and simple, but a choice is never undone, so an early high-priority task can leave a gap that a different combination would have filled.
- **Complexity per slot:** O(k · n · (D + n + E)), where k = tasks placed and D = one Dijkstra run (each pick checks every task: two walking times and a prerequisite check).

### 3.10 0/1 Knapsack dynamic programming (`KnapsackScheduler::planSlots()`)

- **Model per free slot:** capacity = minutes in the slot; item = one ready, reachable task (taken whole or not at all); weight = duration; value = `priority × (slot length + 1) + duration`. The multiplier makes priority decide; duration only breaks ties in favour of using more minutes.
- **Recurrence:**
  ```text
  best[0][w] = 0
  best[i][w] = max( best[i-1][w],                                   skip task i
                    best[i-1][w - weight_i] + value_i )             take task i (if weight_i <= w)
  ```
- **Walk-back:** starting from `best[n][W]`, a task was taken if the value differs from the row above; subtract its weight and continue.
- **Walking time:** it depends on the order, so it cannot be a fixed weight. The chosen tasks are ordered **nearest building next** (ties by `Task::operator>`), and the real route is timed including the walk to the next class. If it is `k` minutes too long, the capacity is reduced by `k` and the DP is solved again (by 1 if a building on the route is unreachable), until the set fits.
- **Property:** the DP gives the best set for the task *durations* within one slot. Walking is added afterwards with a heuristic order, and slots are planned one after another, so the plan is not guaranteed optimal for the whole day. Prerequisites count only if planned in an earlier slot.
- **Complexity per slot:** O(r · (n · C + k² · D)) time, where C = slot minutes and r = number of DP solves; O(n · C) extra space for the table.

---

## 4. Complexity summary

Symbols: n = tasks, E = prerequisite edges, V = buildings, R = walking paths, P = periods, S = free slots, C = minutes in one slot, k = tasks placed in one slot, D = one Dijkstra run.

| Operation | File | Time | Extra space |
|---|---|---|---|
| Heap push / pop | `heap.cpp` | O(log n) | O(1) |
| Heap sort | `heap.cpp` | O(n log n) | O(1) |
| Load tasks (duplicate check per task) | `task_manager.cpp` | O(n²) | O(n) |
| Load timetable (sort after each period) | `schedule_manager.cpp` | O(P² log P) | O(P) |
| Build free slots | `schedule_manager.cpp` | O(P) | O(1) |
| DFS cycle detection | `task_graph.cpp` | O(n + E) | O(n) |
| Kahn's topological sort | `task_graph.cpp` | O((n + E) log n) | O(n) |
| Dijkstra (one query) | `dijkstra.cpp` | O((V + R) log V) | O(V + R) |
| Insertion sort of pending tasks | `workflow_app.cpp` | O(n²) worst, O(n) best | O(n) |
| Greedy, one slot | `optimizer.cpp` | O(k · n · (D + n + E)) | O(1) besides the result |
| Knapsack, one slot | `knapsack.cpp` | O(r · (n · C + k² · D)) | O(n · C) |
| Reasons for unplanned tasks | `scheduler.cpp` | O(n · S · D) | O(1) |

Every walking time is a fresh Dijkstra run (there is no cache). With the project's limits every operation finishes instantly.

---

## 5. Not part of the implementation

An earlier design draft for this project described a hash table with FNV-1a hashing, an indexed heap with `decrease_key`, a Dijkstra result cache, merge sort, binary search and interval merging. **None of these are in the final code.** Building and task lookups use linear search, the heap uses lazy deletion instead of `decrease_key`, timetable sorting uses heap sort, and overlapping periods are rejected rather than merged.
