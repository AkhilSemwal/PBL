#ifndef HEAP_H
#define HEAP_H

// key = priority or distance, value = task id or location id
struct HeapNode
{
    int key;
    int value;
};

enum HeapType
{
    MIN_HEAP,
    MAX_HEAP
};

class Heap
{
private:
    HeapNode* data;
    int count;
    int capacity;
    HeapType type;

    void siftUp(int index);
    void grow();

public:
    explicit Heap(HeapType type, int initialCapacity = 16);
    ~Heap();

    // Heap owns raw memory, copying would cause double delete
    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    void push(int key, int value);
    bool pop(HeapNode& out);
    bool top(HeapNode& out) const;
    bool isEmpty() const;
    int  size() const;
    void clear();
};

void heapSort(HeapNode arr[], int n, bool descending);

#endif
