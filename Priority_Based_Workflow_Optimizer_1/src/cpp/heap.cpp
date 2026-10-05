#include "heap.h"

// Equal keys -> smaller value (id) comes first
static bool maxFirst(const HeapNode& a, const HeapNode& b)
{
    if (a.key != b.key) return a.key > b.key;
    return a.value < b.value;
}

static bool minFirst(const HeapNode& a, const HeapNode& b)
{
    if (a.key != b.key) return a.key < b.key;
    return a.value < b.value;
}

// Heap sort moves root to the end, so root must be the element that belongs last
static bool rootForDescending(const HeapNode& a, const HeapNode& b)
{
    return maxFirst(b, a);
}

static bool rootForAscending(const HeapNode& a, const HeapNode& b)
{
    return minFirst(b, a);
}

typedef bool (*Comparator)(const HeapNode&, const HeapNode&);

static void swapNodes(HeapNode& a, HeapNode& b)
{
    HeapNode temp = a;
    a = b;
    b = temp;
}

static void siftDownArray(HeapNode arr[], int n, int index, Comparator cmp)
{
    while (true)
    {
        int best  = index;
        int left  = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < n && cmp(arr[left], arr[best]))   best = left;
        if (right < n && cmp(arr[right], arr[best])) best = right;

        if (best == index) return;

        swapNodes(arr[index], arr[best]);
        index = best;
    }
}

Heap::Heap(HeapType heapType, int initialCapacity)
{
    type     = heapType;
    capacity = (initialCapacity < 1) ? 1 : initialCapacity;
    count    = 0;
    data     = new HeapNode[capacity];
}

Heap::~Heap()
{
    delete[] data;
}

void Heap::grow()
{
    int newCapacity = capacity * 2;
    HeapNode* newData = new HeapNode[newCapacity];

    for (int i = 0; i < count; ++i)
    {
        newData[i] = data[i];
    }

    delete[] data;
    data = newData;
    capacity = newCapacity;
}

void Heap::siftUp(int index)
{
    Comparator cmp = (type == MAX_HEAP) ? maxFirst : minFirst;

    while (index > 0)
    {
        int parent = (index - 1) / 2;

        if (!cmp(data[index], data[parent])) return;

        swapNodes(data[index], data[parent]);
        index = parent;
    }
}

void Heap::push(int key, int value)
{
    if (count == capacity)
    {
        grow();
    }

    data[count].key   = key;
    data[count].value = value;
    siftUp(count);
    ++count;
}

bool Heap::pop(HeapNode& out)
{
    if (count == 0) return false;

    out = data[0];
    --count;

    if (count > 0)
    {
        data[0] = data[count];
        Comparator cmp = (type == MAX_HEAP) ? maxFirst : minFirst;
        siftDownArray(data, count, 0, cmp);
    }

    return true;
}

bool Heap::top(HeapNode& out) const
{
    if (count == 0) return false;

    out = data[0];
    return true;
}

bool Heap::isEmpty() const
{
    return count == 0;
}

int Heap::size() const
{
    return count;
}

void Heap::clear()
{
    count = 0;
}

void heapSort(HeapNode arr[], int n, bool descending)
{
    if (arr == 0 || n < 2) return;

    Comparator cmp = descending ? rootForDescending : rootForAscending;

    // Build heap from last non-leaf node, O(n)
    for (int i = n / 2 - 1; i >= 0; --i)
    {
        siftDownArray(arr, n, i, cmp);
    }

    // Move root to end, then fix remaining heap
    for (int end = n - 1; end > 0; --end)
    {
        swapNodes(arr[0], arr[end]);
        siftDownArray(arr, end, 0, cmp);
    }
}
