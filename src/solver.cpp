#include "solver.h"
#include <cmath>
#include <cstdlib>

bool solve(Board& board, size_t& solution_count, const bool one_solution, const bool silent) {
	if (board.done()) { // Solved
		if (!silent)
			std::cout << board;
		++solution_count;
		return true;
	}

	// Checkerboard Parity Pruning
	// If the current imbalance (Black - White) is too large to be corrected by
	// the remaining pieces (even if they are placed in their most optimal parity-correcting positions),
	// then no solution is possible.
	// This actually may be removed later since it's unclear if it gives a benefit to speed (even though it should)
	if (std::abs(board.getCurrentImbalance()) > board.getSuffixMaxImbalance())
		return false;

	// Optimization: Only run HSR after a significant part of the board has been filled
	const Tile t = board.getCurrentPiece();
	if (board.openSquares() + t.p_height * t.p_width > 32 && !board.hasSolvableRegions())
		return false;

	const size_t current_piece_index = board.getPieceIndex();
	bool result = false;

	// If the current piece is identical to the previous one,
	// its placement must start after the previous piece's placement because they are grouped together.
	// This prevents solutions with switched identical pieces being considered identical
	size_t start_pos = (current_piece_index > 0 && board.getPiece(current_piece_index) == board.getPiece(current_piece_index - 1)) ? 
					    board.getLastPlacementPos() + 1 : 0;

	// Possible placements and piece bitmasks
	const uint64_t placements = board.placements();
	const uint64_t piece = t.repr;

	// Iterate through all possible top-left placement positions
	const uint8_t start_y = start_pos / 8;
	const uint8_t start_x = start_pos % 8;
	const uint8_t max_y = 7 - t.p_height;
	const uint8_t max_x = 7 - t.p_width;

	for (uint8_t y = start_y; y <= max_y; ++y) {
		uint8_t current_start_x = (y == start_y) ? start_x : 0;
		for (uint8_t x = current_start_x; x <= max_x; ++x) {
			uint8_t i = y * 8 + x;
			// Symmetry breaking for the first piece: restrict to canonical octant
			if (!board.symmetryBroken()) {
				uint8_t y = i / 8;
				uint8_t x = i % 8;
				if (y > 3 || x > 3 || y > x)
					continue;
			}

			const uint64_t placed_piece = piece << i;
			if ((placed_piece & placements) != placed_piece)
				continue;

			board.place(piece, i);
			bool this_result = solve(board, solution_count, one_solution, silent);

			if (this_result) {
				if (one_solution)
					return true;
				result = true;
			}

			board.pop();
		}
	}

	return result;
}