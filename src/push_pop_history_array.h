#ifndef PUSH_POP_HISTORY_ARRAY_H
#define PUSH_POP_HISTORY_ARRAY_H

#include <stdint.h>
#include <cstddef>

class PushPopHistoryArray {
    struct Placement {
        uint64_t occ;
        uint8_t pos; // 0-63 board index of the placement
        int8_t balance_delta;
    };
    Placement history[64];
    size_t used;
public:
    PushPopHistoryArray() : used(0) {}
    size_t size() const { return used; }
    bool empty() const { return !used; }
    const Placement& back() const { return history[used - 1]; }

    const Placement& operator[](const size_t index) const { return history[index]; }
    inline void emplace_back(uint64_t occ, uint8_t pos, int8_t balance_delta) { 
        history[used].occ = occ;
        history[used].pos = pos;
        history[used].balance_delta = balance_delta;
        ++used;
    }
    void pop_back() { if (used > 0) --used; }
};

#endif // PUSH_POP_HISTORY_ARRAY_H