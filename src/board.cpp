#include "board.h"
#include <numeric>
#include <queue>
#include <cmath>

bool Board::use_ansi_colors = false;
bool Board::use_block_characters = false;
bool Board::use_flat_output = false;

// Helper function to get the GCD of a given vector of numbers
uint8_t ListGCD(const std::vector<Tile>& nums) {
    if (nums.empty())
        return 0;
    
    uint8_t g = BIT_COUNT(nums[0].repr);
    for (uint8_t i = 1; i < nums.size(); ++i)
        g = std::gcd(g, static_cast<uint8_t>(BIT_COUNT(nums[i].repr)));

    return static_cast<uint8_t>(g);
}

Board::Board(const std::vector<Tile>& p) {
    occ = 0ULL;
    piece_index = 0;
    current_imbalance = 0;
    pieces = p;
    
    // Precompute minimum remaining piece size
    suffix_min_size.resize(pieces.size() + 1);
    if (!pieces.empty()) {
        int8_t min_sz = 64;
        for (int8_t i = pieces.size() - 1; i >= 0; --i) {
            int8_t count = BIT_COUNT(pieces[i].repr);
            if (count < min_sz)
                min_sz = count;
            suffix_min_size[i] = min_sz;
        }
    }

    // Precompute suffix max imbalance (Checkerboard pruning)
    suffix_max_imbalance.resize(pieces.size() + 1);
    int8_t running_max = 0;
    if (!pieces.empty()) {
        for (int8_t i = pieces.size() - 1; i >= 0; --i) {
            int8_t b = BIT_COUNT(pieces[i].repr & CHECKERBOARD_MASK);
            int8_t w = BIT_COUNT(pieces[i].repr & ~CHECKERBOARD_MASK);
            running_max += std::abs(b - w);
            suffix_max_imbalance[i] = running_max;
        }
    }

    suffix_max_imbalance[pieces.size()] = 0;
    tile_gcd = ListGCD(pieces); // Precompute the GCD for the board
}

// Bitwise floodcount validation
bool Board::hasSolvableRegions() const {
    uint64_t empty = ~occ;
    if (!empty)
        return true;

    int8_t min_sz = suffix_min_size[piece_index];
    if (!min_sz)
        return true; 

    while (empty) {
        uint64_t start_node = (1ULL << LSB(empty));
        uint64_t component = start_node;
        empty &= ~start_node;
        
        while (true) {
            uint64_t grow = component;
            
            grow |= (component & NOT_H_FILE) << 1; // East
            grow |= (component & NOT_A_FILE) >> 1; // West
            grow |= (component << 8);              // South
            grow |= (component >> 8);              // North
            grow |= (component & NOT_H_FILE) << 9; // NE
            grow |= (component & NOT_A_FILE) << 7; // NW
            grow |= (component & NOT_H_FILE) >> 7; // SE
            grow |= (component & NOT_A_FILE) >> 9; // SW
            
            uint64_t new_nodes = grow & empty;
            if (!new_nodes)
                break;
            
            component |= new_nodes;
            empty &= ~new_nodes;
        }
        
        if (BIT_COUNT(component) < min_sz
            || BIT_COUNT(component) % tile_gcd != 0)
            return false;
    }

    return true;
}

uint64_t Board::complexityScore() const {
    uint64_t score = 0;
    uint64_t temp = 1ULL << 63;
    for (size_t i = 0; i < 64; ++i) {
        if (temp) {
            if (occ & (1ULL << i))
                temp >>= 1;
            else {
                score += temp;
                temp = 1ULL << 63;
            }
        }
    }

    if (temp != 1ULL << 63)
        score += temp;
        
    return score;
}

char Board::getChar(const uint8_t x, const uint8_t y) const {
    uint64_t mask = 1ULL << (y * 8 + x);
    if (!(occ & mask))
        return '.';
    
    // Reconstruct which piece is here
    for (size_t i = 0; i < piece_index; ++i)
        if ((pieces[i].repr << history[i].pos) & mask)
            return 'a' + (i % 26);
    
    return '?';
}

const char* ANSI_RESET = "\033[0m";
const char* ANSI_COLORS[] = {
    "\033[31m", // Red
    "\033[32m", // Green
    "\033[33m", // Yellow
    "\033[34m", // Blue
    "\033[35m", // Magenta
    "\033[36m", // Cyan
    "\033[91m", // Bright Red
    "\033[92m", // Bright Green
    "\033[93m", // Bright Yellow
    "\033[94m", // Bright Blue
    "\033[95m", // Bright Magenta
    "\033[96m", // Bright Cyan
};
const char* ANSI_BACKGROUND_COLORS[] = {
    "\033[41m", // Red background
    "\033[42m", // Green background
    "\033[43m", // Yellow background
    "\033[44m", // Blue background
    "\033[45m", // Magenta background
    "\033[46m", // Cyan background
    "\033[101m", // Bright Red background
    "\033[102m", // Bright Green background
    "\033[103m", // Bright Yellow background
    "\033[104m", // Bright Blue background
    "\033[105m", // Bright Magenta background
    "\033[106m", // Bright Cyan background
};
const uint8_t NUM_COLORS = 12;

std::ostream& operator<<(std::ostream& out, const Board& board) {
    char buffer[745]; // Max board output size including ANSI characters
    size_t ptr = 0;
    
    if (!Board::use_flat_output) {
        const char* header = "Board:\n";
        while (*header)
            buffer[ptr++] = *header++;
    }
    
    for (uint8_t y = 0; y < 8; ++y) {
        if (!Board::use_flat_output)
            buffer[ptr++] = '\t';
        for (uint8_t x = 0; x < 8; ++x) {
            char piece_char = board.getChar(x, y);
            if (Board::use_ansi_colors && piece_char >= 'a' && piece_char <= 'z') {
                uint8_t color_idx = static_cast<int8_t>(piece_char - 'a') % NUM_COLORS;
                const char* color = Board::use_block_characters ? ANSI_BACKGROUND_COLORS[color_idx] : ANSI_COLORS[color_idx];

                while (*color)
                    buffer[ptr++] = *color++;

                if (Board::use_block_characters) {
                    buffer[ptr++] = ' ';
                    buffer[ptr++] = ' ';
                }
                else
                    buffer[ptr++] = piece_char;

                const char* reset = ANSI_RESET;
                while (*reset)
                    buffer[ptr++] = *reset++;
            } else {
                if (Board::use_block_characters && piece_char == '.') {
                    buffer[ptr++] = ' ';
                    buffer[ptr++] = ' ';
                }
                else
                    buffer[ptr++] = piece_char;
            }
            if (!Board::use_block_characters && !Board::use_flat_output)
                buffer[ptr++] = ' ';
        }
        if (!Board::use_flat_output)
            buffer[ptr++] = '\n';
    }
    if (Board::use_flat_output)
        buffer[ptr++] = '\n';

    buffer[ptr] = '\0';
    out << buffer;
    return out;
}