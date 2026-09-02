# Data Structures & Algorithms (DSA) Design

This document details the design, rationale, and complexity profiles of every custom data structure and foundational algorithm implemented in ChronoSpatial's C core layer.

---

## 1. Custom Data Structures

### 1.1 Singly-Linked List (`linked_list.h`, `linked_list.c`)
- **Purpose**: Implements the bucket chain for the hash map to resolve key collisions via separate chaining.
- **Node Structure**:
  ```c
  typedef struct ListNode {
      char*            key;     // Dynamically allocated string key
      int              value;   // Mapped integer value (e.g. vertex ID)
      struct ListNode* next;    // Pointer to successor node
  } ListNode;
  ```
- **Operations & Complexity**:
  - `list_push_front`: $O(1)$ prepend; updates existing node in place if key matches.
  - `list_find`: $O(k)$ where $k$ is the length of the chain.
  - `list_remove`: $O(k)$ search and unlink.
  - `list_destroy`: $O(k)$ deallocation of all chain nodes and duplicate strings.

---

### 1.2 Hash Table with Separate Chaining (`hash_table.h`, `hash_table.c`)
- **Purpose**: Translates textual location keys (e.g., `"cs_lab"`, `"main_library"`) to continuous integer vertex IDs ($0 \dots V-1$) in $O(1)$ expected time.
- **Hash Function (FNV-1a 64-bit)**:
  ```c
  unsigned long long h = 14695981039346656037ULL;
  while (*key) {
      h ^= (unsigned char)*key++;
      h *= 1099511628211ULL;
  }
  ```
  Provides strong avalanche properties on short ASCII identifiers with minimal distribution clustering.
- **Dynamic Resizing (Rehashing)**:
  - Triggered when load factor $\alpha = \frac{\text{size}}{\text{buckets}} > 0.75$.
  - Doubles the bucket count and re-inserts all active nodes without heap reallocations of string buffers.
- **Complexity**:
  - Insert / Lookup / Delete: $O(1)$ expected, $O(n)$ worst-case.
  - Space: $O(N + B)$ where $B$ is the bucket array size.

---

### 1.3 Indexed Binary Min-Heap (`priority_queue.h`, `priority_queue.c`)
- **Purpose**: Core priority queue powering Dijkstra's single-source shortest path algorithm.
- **Why Indexed Heap?**:
  Standard priority queues require $O(V)$ linear scans for key updates or lazy deletion which inflates queue size to $O(E)$. The indexed min-heap maintains three parallel arrays:
  1. `heap[i]`: Maps heap slot $i \to$ vertex ID.
  2. `pos[v]`: Maps vertex ID $v \to$ current slot index in `heap[]` (or `-1` if absent).
  3. `key[v]`: Maps vertex ID $v \to$ current shortest distance.
- **Invariant**:
  $$\text{pos}[\text{heap}[i]] = i \quad \forall \; 0 \le i < \text{size}$$
- **Operations & Complexity**:
  - `pq_contains(v)`: $O(1)$ by testing `pos[v] != -1`.
  - `pq_decrease_key(v, new_dist)`: $O(\log V)$ via direct lookup in `pos[v]` followed by `sift_up`.
  - `pq_extract_min()`: $O(\log V)$ via root extraction and `sift_down`.
  - `pq_push()`: $O(\log V)$.
  - Total Memory: Fixed $O(V)$, never grows with edge count $E$.

---

### 1.4 Weighted Graph (`graph.h`, `graph.c`)
- **Purpose**: Models campus spatial topology. Vertices represent buildings/landmarks; undirected edges represent footpaths weighted by walking minutes.
- **Representation**: Adjacency list using dynamically resizing edge arrays.
- **Complexity**:
  - Space: $O(V + E)$.
  - Edge Traversal: $O(\text{deg}(v))$ instead of $O(V)$ required by adjacency matrices.

---

## 2. Core Algorithmic Profiling

| Algorithm | Component | Time Complexity | Space Complexity | Guarantees / Properties |
| :--- | :--- | :--- | :--- | :--- |
| **Dijkstra's SSSP** | `dijkstra.c` | $O((V + E) \log V)$ | $O(V)$ | Optimal shortest paths; cached per source vertex |
| **Top-Down Merge Sort** | `sorting.c` | $O(n \log n)$ | $O(n)$ | Strictly stable; no worst-case degradation |
| **Interval Sweep** | `Schedule.cpp` | $O(n)$ | $O(n)$ | Merges overlapping intervals in a single linear pass |
| **0/1 Knapsack DP** | `TimeOptimizer.cpp`| $O(k \cdot C)$ | $O(k \cdot C)$ | Global mathematical optimum with multi-criteria tie-break |
| **Binary Search** | `searching.c` | $O(\log n)$ | $O(1)$ | Fast exact lookup on sorted schedules |
