# Priority Based Workflow Optimizer for Efficient Time Utilization

> **Turning dead time between fixed commitments into mathematically optimized, planned time.**

**Priority Based Workflow Optimizer for Efficient Time Utilization** bridges physical campus spatial topology with temporal constraint optimization. Built in standard **C99** (core data structures & algorithms) and **C++17** (object-oriented application logic) with **zero external dependencies**.

---

## 1. Features & Architecture

```text
Priority Based Workflow Optimizer for Efficient Time Utilization Architecture
├── include/
│   ├── c/                  # Pure C data structure headers
│   │   ├── linked_list.h   # Singly-linked list for hash table buckets
│   │   ├── hash_table.h    # 64-bit FNV-1a hash map with separate chaining
│   │   ├── priority_queue.h# Indexed binary min-heap (O(log V) decrease-key)
│   │   ├── graph.h         # Adjacency-list weighted undirected graph
│   │   ├── dijkstra.h      # Shortest path solver with per-source memoization
│   │   ├── sorting.h       # Stable top-down merge sort (O(n log n))
│   │   └── searching.h     # Binary search & lower bound
│   └── cpp/                # C++ OOP application headers
│       ├── Location.h      # Campus vertex representation & tag matching
│       ├── Task.h          # Knapsack item (minutes, priority, venue constraints)
│       ├── FixedActivity.h # Mandatory fixed commitments (classes, labs)
│       ├── Schedule.h      # Timeline sweep & gap extraction
│       ├── RouteManager.h  # Graph & Dijkstra C++ bridge
│       ├── TaskManager.h   # Flexible task pool & venue filtering
│       ├── TimeOptimizer.h # 0/1 knapsack DP, greedy, & brute-force oracle
│       ├── Itinerary.h     # Optimized plan data structures
│       ├── FileManager.h   # Line-level diagnostic parser for data files
│       └── PriorityBasedWorkflowOptimizerSystem.h # Top-level orchestrator & interactive CLI
├── src/
│   ├── c/                  # Pure C implementations
│   ├── cpp/                # C++ OOP implementations
│   └── main.cpp            # Application entry point
├── data/
│   ├── locations.txt       # Campus venues with capability tags
│   ├── routes.txt         # Footpaths with walking durations (minutes)
│   ├── schedule.txt       # Daily fixed commitments
│   ├── tasks.txt           # Flexible tasks with priority points
│   └── config.txt          # Planner configuration (safety buffer, min gap)
├── docs/                   # Complete project documentation
│   ├── architecture.md    # System design & layer separation
│   ├── dsa_design.md      # Custom DSA implementation details & complexities
│   ├── oop_design.md      # Class hierarchy & design patterns
│   ├── optimization.md    # Mathematical knapsack formulation & proofs
│   ├── algorithms.md      # Algorithmic pseudocode & Never-Late proof
│   └── test_cases.md      # Automated testing suite & coverage
└── tests/                  # Unit and verification tests
    ├── test_hash_table.c
    ├── test_priority_queue.c
    ├── test_sorting.c
    ├── test_graph.c
    ├── test_dijkstra.c
    └── test_optimizer.cpp
```

---

## 2. Quick Start & Build

### Prerequisites

* `gcc` and `g++` supporting C99 and C++17.
* `make`.

### Build & Run

```bash
# 1. Compile everything into bin/priority_workflow_optimizer
make all

# 2. Run the interactive console menu
make run

# 3. Print the optimized itinerary directly
make plan

# 4. Print the algorithmic complexity and DP vs Greedy benchmark report
make report

# 5. Run the full automated verification test suite
make test
```

---

## 3. The 4-Step Algorithmic Pipeline

### 1. Timeline Sweep (Merge Sort + Interval Merge)

Takes unsorted fixed events, sorts them in **O(n log n)** time, merges overlapping commitments in an **O(n)** sweep, and extracts idle gaps.

### 2. Shortest-Path Spatial Transit (Dijkstra)

Uses an indexed binary min-heap with **O(log V)** priority updates to compute minimum walking time between any pair of venues. Distances are memoized per source vertex.

### 3. True Usable Time & Spatio-Temporal Venue Search

For every idle gap between events at locations `u` and `w`, tests each campus venue `v`:

$$
\text{True Usable Time}
=
\text{GapDuration}
-
\text{walk}(u,v)
-
\text{walk}(v,w)
-
\text{SafetyBuffer}
$$

This ensures that the time allocated to a task accounts for both the journey to the selected venue and the journey to the next fixed commitment.

### 4. Multi-Criteria 0/1 Knapsack Dynamic Programming

Fills the exact usable-time capacity at each venue with the optimal subset of eligible flexible tasks, beating greedy heuristics and guaranteeing the user is **never late** to subsequent commitments.

---

## 4. Documentation

For comprehensive technical documentation, refer to the `docs/` directory:

* **Architecture & Data Flow** — `docs/architecture.md`
* **Data Structures & Complexity Analysis** — `docs/dsa_design.md`
* **OOP Design & Patterns** — `docs/oop_design.md`
* **Optimization Model & Greedy Counterexamples** — `docs/optimization.md`
* **Algorithms & Never-Late Proof** — `docs/algorithms.md`
* **Test Strategy & Verification** — `docs/test_cases.md`
