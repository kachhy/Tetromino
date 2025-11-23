#ifndef BOARD_H
#define BOARD_H

#include "push_pop_history_array.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>

#define BIT_COUNT(b)  (__builtin_popcountll(b))
#define POP_BIT(b, i) (b & ~(1ULL << i))
#define MSB(b)        (63 - __builtin_clzll(b))
#define LSB(b)        (__builtin_ctzll(b))

#define CHECKERBOARD_MASK (0xAA55AA55AA55AA55ULL)
#define NOT_A_FILE        (0xFEFEFEFEFEFEFEFEULL)
#define NOT_H_FILE        (0x7F7F7F7F7F7F7F7FULL)

struct Tile {
    uint64_t repr;
    uint8_t p_width, p_height;

    Tile(const uint64_t r) {
        repr = r;
        uint8_t max_x = 0;
        uint8_t max_y = 0;
        for (int i = 0; i < 64; ++i) {
            if ((r >> i) & 1) {
                uint8_t x = i % 8;
                uint8_t y = i / 8;
                if (x > max_x) max_x = x;
                if (y > max_y) max_y = y;
            }
        }
        p_width = max_x;
        p_height = max_y;
    }

    bool operator==(const Tile& other) const {
        return repr == other.repr;
    }
};

class Board {
    uint64_t occ;
    PushPopHistoryArray history;
    std::vector<Tile> pieces;
    std::vector<uint8_t> suffix_min_size;
    std::vector<int> suffix_max_imbalance;
    size_t piece_index;
    uint8_t tile_gcd;
    int current_imbalance;

public:
    Board() = delete;
    Board(const std::vector<Tile>& p);

    constexpr char currentPieceChar() const { return 'a' + piece_index; }
    Tile getCurrentPiece() const { return pieces[piece_index]; }
    Tile getPiece(size_t index) const { return pieces[index]; }
    constexpr size_t getPieceIndex() const { return piece_index; }
    uint8_t getLastPlacementPos() const { return history.empty() ? 0 : history.back().pos; }
    constexpr uint64_t placements() const { return ~occ; }
    constexpr size_t hash() const { return occ; }
    bool done() const { return piece_index == pieces.size(); }
    char getChar(const uint8_t x, const uint8_t y) const;
    size_t numPieces() const { return pieces.size(); }
    int getSuffixMaxImbalance() const { return suffix_max_imbalance[piece_index]; }
    int getCurrentImbalance() const { return current_imbalance; }

    inline void place(const uint64_t piece, const uint8_t pos) {
        const uint64_t p = piece << pos;
        const uint8_t black = BIT_COUNT(p & CHECKERBOARD_MASK);
        const uint8_t white = BIT_COUNT(p & ~CHECKERBOARD_MASK);
        const int8_t delta = static_cast<int8_t>(black - white);

        history.emplace(occ, pos, delta); // Store current occ and new position
        occ |= p;
        current_imbalance += delta;
        ++piece_index;
    }
    inline void pop() {
        if (history.empty())
            return;
        
        const auto& last = history.back();
        occ = last.occ;
        current_imbalance -= last.balance_delta;
        history.pop();
        --piece_index;
    }

    bool hasSolvableRegions() const;

    bool operator==(const Board& other) const { return occ != other.occ; }

    // For color printing to make output readable :)
    static bool use_ansi_colors;
    static void setUseColor(bool enable) { use_ansi_colors = enable; }
    static bool use_block_characters;
    static void setUseBlockCharacters(bool enable) { use_block_characters = enable; }
    static bool use_flat_output;
    static void setUseFlatOutput(bool enable) { use_flat_output = enable; }
};

template<> struct std::hash<Board> {
    std::size_t operator()(Board const& b) const noexcept {
        return b.hash();
    }
};

std::ostream& operator<<(std::ostream& out, const Board& board);

#endif // BOARD_H