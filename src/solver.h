#ifndef SOLVER_H
#define SOLVER_H

#include <unordered_set>
#include "board.h"

bool solve(Board& board, size_t& solution_count, const bool one_solution, const bool silent);

#endif // SOLVER_H