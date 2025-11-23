# Tetromino
Reasonably fast 8x8 Tetromino solver

## What is this?

This is a solver for an 8x8 tetromino game. The solver is passed a file with a set of tiles that fit on an 8x8 board, and it tries to find solutions with all tiles on the board.

## Building
To build the program, run (in the src directory)
```sh
g++ *.cpp -o solver -O3 -flto -march=native
```

## Usage
To use the solver, run the solver
```sh
./solver <input_file>
```
and to enable finding all unique solutions, run
```sh
./solver <input_file> --all-solutions
```
You can even enable a task-queue based multithreading mode with 
```sh
./solver <input_file> --threads <number_of_threads>
```

There are a few command options to make the whole thing a bit prettier, the first is coloring each tile with
```sh
./solver <input_file> --color
```
And with the color mode, you can enable block printing with
```sh
./solver <input_file> --color --blocks
```
These only work with an ANSI-compatible terminal. To get a flat output (one solution per line for reduced validation) run
```sh
./solver <input_file> --flat
```
And if you want no output except the number of solutions found (with all solutions enabled), use
```sh
./solver <input_file> --silent
```

## Test cases
I've included a few test cases in the tests folder. The solver can solve all of these without multithreading, but some tests are designed specifically to test multithreading perforamance, such as 
- complexheavy.txt
- complexextreme.txt
- rowsheavy.txt
- blocksextreme3.txt

Some good general test cases are 
- everything.txt
- blocksheavy.txt
- complex.txt
- blocks.txt
- blockssuper.txt

## Technical stuff
Here are some more technical specifications for the solver.
### Optimizations
- Bitboard representation of the tetromino board, allowing for blazingly fast occupancy checks and updates
- A bitwise flood fill board checker ensures boards with impossible regions are backtracked immediately
    - Additionally, we precompute the GCD of every piece to ensure that areas are divisible by the pieces provided
- Checkerboard parity checking ensures that impossible solutions due to parity of coloring are pruned at all search depths
- Symmetry breaking by fixing the first tile in the canonical octant, as flips and rotations and flips are considered non-unique
- Restrictive tile grouping reduces the search tree earlier
- We avoid ever copying the board using a push-pop board design
### Puzzle file format
The puzzle file format itself is pretty simple. Each tile is on its own line, with each piece formed by a few sub-tiles, placed one-by one with (x, y) coordinates, deliniated by spaces. An example T-piece is 
```
(0,0) (1,0) (2,0) (1,1)
```
All tiles should start at (0,0) unless it is impossible to do so. Each file can contain 1-64 tiles. There are many valid puzzle inputs, and you can see some examples in the test files.

Note: all tiles must fit on the 8x8 board, and no coordinate can go outside the bounds [0, 7].
