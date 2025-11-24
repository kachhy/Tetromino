#include <fstream>
#include <thread>
#include <atomic>
#include "solver.h"

// Global synchronization
std::atomic<size_t> solution_count{0};
std::atomic<size_t> next_task_index{0};
std::atomic<bool> finished{false};

// Shared task queue and silent status
std::vector<Board> task_queue;
bool silent = false;

void worker_thread(const bool one_solution) {
    size_t local_sol_count = 0;

    while (!finished) {
        size_t my_task_idx = next_task_index.fetch_add(1); // Grab the next task
        
        if (my_task_idx >= task_queue.size())
            break;

        // Make a local copy to work on
        Board board = task_queue[my_task_idx];
        size_t internal_count = 0;
        bool result = solve(board, internal_count, one_solution, silent);

        local_sol_count += internal_count;

        if (one_solution && result)
            finished = true;
    }

    solution_count += local_sol_count;
}

void generateTasks(Board& board, uint8_t depth, const uint8_t goal_depth) {
    // If we have generated enough depth (2 pieces placed) or run out of pieces,
    // save the state as a task.
    // We target depth 2 (placing piece 0 and piece 1) to create enough granularity.
    if (depth == goal_depth || board.done()) {
        task_queue.push_back(board);
        return;
    }

    const size_t current_piece_index = board.getPieceIndex();
    const uint64_t placements = board.placements();
    const Tile t = board.getCurrentPiece();
    const uint64_t piece = t.repr;
    size_t start_pos = 0;

    // Identical piece ordering constraint
    if (current_piece_index > 0 && board.getPiece(current_piece_index) == board.getPiece(current_piece_index - 1))
        start_pos = board.getLastPlacementPos() + 1;

    for (size_t i = start_pos; i < 64; ++i) {
        // Symmetry breaking for the VERY first piece (Piece 0 only)
        if (current_piece_index == 0) {
            uint8_t y = i / 8;
            uint8_t x = i % 8;
            if (y > 3 || x > 3 || y > x)
                continue;
        }

        // Boundary check
        if ((i % 8) > static_cast<uint8_t>(7 - t.p_width) || (i / 8) > static_cast<uint8_t>(7 - t.p_height))
            continue;

        // Collision check
        const uint64_t placed_piece = piece << i;
        if ((placed_piece & placements) != placed_piece)
            continue;

        board.place(piece, static_cast<uint8_t>(i));
        generateTasks(board, depth + 1, goal_depth);
        board.pop();
    }
}

void threadManager(const std::vector<Tile>& tiles, const bool one_sol, const size_t num_threads) {
    Board board(tiles);
    
    // Reset globals
    solution_count = 0;
    next_task_index = 0;
    finished = false;
    task_queue.clear();
    
    // Single threaded or multi threaded?
    if (num_threads == 0 || num_threads == 1) {
        // We can just run the tasks sequentially
        size_t dummy_count = 0;
        bool found_solution = solve(board, dummy_count, one_sol, silent);

        if (!found_solution)
            std::cout << "No solutions." << std::endl;
        else if (!one_sol)
            std::cout << "\nFound " << dummy_count << (dummy_count == 1 ? " solution." : " solutions.") << std::endl;
    } else {
        // Pre-generate tasks by expanding the first few levels of the tree
        // If we only have 1 piece, depth 2 will cover it (depth 1 logic handles board.done())
        generateTasks(board, 0, 2); // TODO: we can implement dynamic depth scaling
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (size_t i = 0; i < num_threads; ++i)
            threads.emplace_back(worker_thread, one_sol);

        for (std::thread& t : threads)
            t.join();
        
        if (!solution_count)
            std::cout << "No solutions." << std::endl;
        else
            std::cout << "\nFound " << solution_count << (solution_count == 1 ? " solution." : " solutions.") << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc < 2) { 
        std::cerr << "Usage: " << argv[0] << " <tile file> [--all-solutions] [--threads <num_threads>] [--color] [--blocks] [--silent] [--flat]" <<  std::endl;
        return 1;
    }

    // Argument parsing
    std::string input_file = argv[1];
    size_t threads = 0;
    bool one_sol = true;

    for (uint8_t i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--all-solutions")
            one_sol = false;
        else if (arg == "--color")
            Board::setUseColor(true);
        else if (arg == "--blocks")
            Board::setUseBlockCharacters(true);
        else if (arg == "--silent")
            silent = true;
        else if (arg == "--flat")
            Board::setUseFlatOutput(true);
        else if (arg == "--threads") {
            if (i + 1 < argc)
                threads = static_cast<size_t>(atoi(argv[++i]));
            else {
                std::cerr << "Error: --threads requires a number." << std::endl;
                return 1;
            }
        }
    }

    if (!Board::use_ansi_colors && Board::use_block_characters) {
        std::cerr << "WARNING: --blocks must be used with the --color argument." << std::endl;
        Board::setUseBlockCharacters(false);
    }

    std::ifstream in(input_file);
    if (!in.good()) {
        std::cerr << "Error: Unable to open input file \"" << input_file << "\"." << std::endl;
        return 1;
    }

    // Load each tile, one by one
    std::vector<Tile> tiles;
    std::string line;
    while (std::getline(in, line)) {
        uint64_t tile = 0ULL;
        size_t pos = 0;
        while ((pos = line.find('(', pos)) != std::string::npos) {
            const size_t end_x = line.find(',', pos + 1);
            const size_t end_y = line.find(')', end_x + 1);
            
            // Malformed coordinate
            if (end_x == std::string::npos || end_y == std::string::npos) {
                std::cerr << "Warning: Found malformed coordinate" << std::endl;
                pos++;
                continue;
            }

            int8_t x = std::stoi(line.substr(pos + 1, end_x - pos - 1));
            int8_t y = std::stoi(line.substr(end_x + 1, end_y - end_x - 1));

            if (x >= 0 && x < 8 && y >= 0 && y < 8)
                tile |= (1ULL << (y * 8 + x));
            else
                std::cerr << "Warning: Coordinate (" << x << "," << y << ") out of 8x8 board bounds." << std::endl;

            pos = end_y + 1;
        }

        if (tile)
            tiles.emplace_back(tile);
    }

    if (tiles.empty()) {
        std::cerr << "Error: No valid tiles found in input file." << std::endl;
        return 1;
    }

    // Sort tiles by most restrictive placement, and group identicals
    std::sort(tiles.begin(), tiles.end(), [](Tile a, Tile b) {
        uint8_t count_a = BIT_COUNT(a.repr);
        uint8_t count_b = BIT_COUNT(b.repr);
        if (count_a != count_b)
            return count_a > count_b;

        // If sizes are the same, sort by the bitmask value to group identical tiles
        return a.repr > b.repr;
    });

    // Start our solver
    threadManager(tiles, one_sol, threads);

    return 0;
}
