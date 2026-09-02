# Object-Oriented Design (OOP) Architecture

This document describes the Object-Oriented design principles, class hierarchy, interfaces, and design patterns utilized in ChronoSpatial's C++ application layer.

---

## 1. Design Principles

1. **Single Responsibility Principle (SRP)**:
   - `Location`: Manages identity and tags of a place.
   - `Task`: Manages task priority, duration, and constraints.
   - `FixedActivity`: Encapsulates scheduled temporal blocks.
   - `Schedule`: Handles timeline manipulation, sorting, and gap extraction.
   - `RouteManager`: Bridges C graph/shortest-path algorithms to C++.
   - `TimeOptimizer`: Implements knapsack DP and heuristics.
   - `FileManager`: Encapsulates I/O and text parsing.
   - `ChronoSpatialSystem`: High-level workflow orchestration and CLI/UI rendering.

2. **Encapsulation & Immutability**:
   - Time calculations are strictly performed using integer minutes since midnight to avoid floating-point inaccuracies.
   - Domain objects return immutable references or values where appropriate.

3. **Bridge & Adapter Pattern**:
   - `RouteManager` acts as an Object Adapter over the pure C `Graph` and `DijkstraSolver` subsystems, translating C pointers/arrays to modern C++ vectors and string objects with automatic RAII memory management.

4. **Strategy Pattern**:
   - `TimeOptimizer` supports multiple packing strategies (Exact 0/1 DP, Greedy Value-Density Heuristic, and Exhaustive Brute-Force Oracle) sharing identical inputs.

---

## 2. Class Class Diagram & Relationships

```
                  +-----------------------+
                  |  ChronoSpatialSystem  |
                  +-----------+-----------+
                              |
       +--------------+-------+-------+--------------+
       |              |               |              |
       v              v               v              v
+--------------+ +----------+ +---------------+ +---------------+
| RouteManager | | Schedule | |  TaskManager  | | TimeOptimizer |
+-------+------+ +----+-----+ +-------+-------+ +---------------+
        |             |               |
        v             v               v
   [Location]  [FixedActivity]     [Task]
```

---

## 3. Key Classes Breakdown

### 3.1 `RouteManager`
- **Responsibilities**: Maintains location lookup, coordinates with C-layer `graph` and `dijkstra` structures, manages shortest-path memoization.
- **Key Methods**:
  - `addLocation(key, name, tags)`: Registers venue.
  - `addPath(keyA, keyB, minutes)`: Creates bidirectional edge.
  - `walkMinutes(from, to)`: Returns memoized shortest walking time.
  - `shortestPath(from, to)`: Reconstructs path vertex sequence.

### 3.2 `Schedule`
- **Responsibilities**: Manages the day's fixed commitments.
- **Key Methods**:
  - `sortByStart()`: Triggers stable C merge-sort on events.
  - `busyBlocks()`: Performs interval sweep to merge overlapping commitments.
  - `findGaps(minMinutes)`: Extracts free time windows between busy blocks.
  - `findConflicts()`: Identifies schedule overlaps.

### 3.3 `TaskManager`
- **Responsibilities**: Holds flexible task pool, filters candidates by venue capability tags.
- **Key Methods**:
  - `eligibleAt(locationId, routes)`: Identifies unspent tasks valid at a given venue.
  - `markScheduled(taskIndex)`: Marks a task as spent.
  - `resetAll()`: Resets task allocation for idempotent re-runs.

### 3.4 `TimeOptimizer`
- **Responsibilities**: Executes combinatorial optimization over task subsets for a target capacity.
- **Key Methods**:
  - `packTasks(tasks, candidates, capacity)`: Returns optimal subset using 2D Dynamic Programming.
  - `packTasksGreedy(...)`: Executes density-based greedy packing for performance comparison.
  - `packTasksBruteForce(...)`: Exhaustive $O(2^k)$ search for unit testing validation.
