# ChronoSpatial :: System Architecture

## 1. Overview
ChronoSpatial is a spatio-temporal daily schedule optimizer designed to eliminate "dead time" in busy daily schedules (such as campus university timetables). Traditional calendar software only tracks *when* events occur, ignoring physical transit constraints and location-specific task suitability. ChronoSpatial combines graph algorithms and dynamic programming to calculate exact **True Usable Time** within idle gaps and assign optimal task subsets with a mathematical **Never-Late Guarantee**.

```
+-----------------------------------------------------------------------------------+
|                              CHRONOSPATIAL SYSTEM                                 |
|                                                                                   |
|  +--------------------+   +-----------------------+   +------------------------+  |
|  |    Campus Map      |   |     Fixed Schedule    |   |     Flexible Tasks     |  |
|  | (locations/routes) |   |    (classes/labs)     |   |   (coursework/errands) |  |
|  +---------+----------+   +-----------+-----------+   +-----------+------------+  |
|            |                          |                           |               |
|            v                          v                           |               |
|     [RouteManager]             [DaySchedule]                      |               |
|     (Dijkstra memo)           (MergeSort+Sweep)                   |               |
|            |                          |                           |               |
|            |       +------------------+                           |               |
|            |       |                                              |               |
|            v       v                                              v               |
|      +-----------------------------------------------------------------+          |
|      |                        TimeOptimizer (0/1 DP)                   |          |
|      |        True Usable Time = Gap - WalkOut - WalkBack - Buffer     |          |
|      +--------------------------------+--------------------------------+          |
|                                       |                                           |
|                                       v                                           |
|                           [Itinerary & Reports]                                   |
|                        (Never-Late Day Schedule)                                  |
+-----------------------------------------------------------------------------------+
```

---

## 2. Two-Layer Architecture

The project is architected with a strict separation between fundamental Data Structures & Algorithms (pure C layer) and domain application logic (C++ Object-Oriented layer).

### Layer 1: Core C DSA Layer (`include/c/`, `src/c/`)
Implemented strictly in standard C (C99), providing foundational data structures and classical graph/sorting algorithms with zero external dependencies:
- **`linked_list`**: Singly-linked list with key-value pairs for separate chaining collision resolution.
- **`hash_table`**: Hash map with 64-bit FNV-1a hashing and automatic load-factor rehashing at 0.75 threshold.
- **`priority_queue`**: Indexed binary min-heap with $O(\log V)$ `decrease_key` and $O(1)$ `contains`.
- **`graph`**: Weighted undirected graph represented as an adjacency list with $O(V+E)$ space.
- **`dijkstra`**: Single-source shortest path solver with per-source memoization cache.
- **`sorting`**: Guaranteed $O(n \log n)$ stable top-down merge sort.
- **`searching`**: Binary search and lower-bound utilities on contiguous memory.

### Layer 2: C++ OOP Layer (`include/cpp/`, `src/cpp/`)
Encapsulates domain concepts and coordinates the algorithmic pipeline:
- **`Location`**: Value object encapsulating vertex keys, display names, and venue capability tags.
- **`Task`**: Flexible activity model encapsulating minutes (weight), priority (value), and venue constraints.
- **`FixedActivity`**: Immovable scheduled event with integer-minute timestamps since midnight.
- **`Schedule`**: Manager for the fixed daily timeline; executes merge-sorting, interval merging, and idle gap extraction.
- **`RouteManager`**: Wraps the C graph and Dijkstra solver, providing campus distance calculations and location tag filtering.
- **`TaskManager`**: Manages the candidate task pool and handles state transitions (scheduled/unscheduled).
- **`TimeOptimizer`**: Implements exact 0/1 knapsack dynamic programming, greedy heuristic, and exhaustive brute-force oracle.
- **`Itinerary`**: Aggregates the optimized day plan, gap timing, walk legs, and summary statistics.
- **`FileManager`**: Reads pipe-delimited data files (`locations.txt`, `routes.txt`, `schedule.txt`, `tasks.txt`, `config.txt`) with line-level error tracing.
- **`ChronoSpatialSystem`**: Top-level orchestrator connecting all subsystems, managing CLI options and interactive console menus.

---

## 3. Data Flow and Execution Pipeline

```
1. Parse Data Files (FileManager)
   ├── locations.txt -> RouteManager (Graph vertices)
   ├── routes.txt    -> RouteManager (Graph edges)
   ├── schedule.txt  -> Schedule (Fixed activities)
   └── tasks.txt     -> TaskManager (Flexible task pool)
           │
2. Timeline Sweep (Schedule)
   ├── Stable Merge Sort by start time: O(n log n)
   ├── Interval Sweep (Merge overlapping busy blocks): O(n)
   └── Idle Gap Extraction: O(n)
           │
3. Spatio-Temporal Gap Optimization (ChronoSpatialSystem & TimeOptimizer)
   For each IdleGap (fromLocation -> toLocation):
     For each Candidate Venue on campus:
       ├── Calculate WalkOut = RouteManager::walkMinutes(fromLocation, venue)
       ├── Calculate WalkBack = RouteManager::walkMinutes(venue, toLocation)
       ├── UsableTime = GapDuration - WalkOut - WalkBack - SafetyBuffer
       ├── If UsableTime > 0:
       │     CandidateTasks = TaskManager::eligibleAt(venue)
       │     PackResult = TimeOptimizer::packTasks(CandidateTasks, UsableTime)
       └── Keep (venue, task_subset) pair maximizing (Value, MinutesUsed, -WalkTime)
           │
4. Itinerary Synthesis
   ├── Assign exact activity timestamps starting upon arrival at venue
   ├── Calculate departure time: GapEnd - WalkBack - SafetyBuffer
   └── Generate verifiable Never-Late timeline
```
