# Threading strategy

The main thread must be the one handling the platform specific windowing and input data. Both glfw and Cocoa disallow most operations from the threads different from the main one.

Threads:
  - 0 - Main thread: Windowing, input, platform specific stuff 
  - 1 - Logic thread: Game logic, executes game logic and produces renderlists
  - 2 - Rendering thread: Draws renderlists and presents
  - n-3 - Jobs executors:

Possible buffer states:
  - Written from A
  - Read from B


Cycle vector:
|------|------|------|
| read | ready|  wip |
|------|------|------|
 v              ^
 Cons           Prod

Function:
  - Prod is done writing wip
  - Prod checks if the next cell is free
  - Prod sees that the next cell is used by cons
  - Prod rewrites wip
    |------|------|------|
    |read  |ready |  wip |
    |------|------|------|
     v              ^
     Cons           Prod
  - Cons is done with its cell. Checks if the next cell is free. It is free
  - Cons starts consuming the next cell.
    |------|------|------|
    |empty | read |  wip |
    |------|------|------|
             v      ^
             Cons   Prod
  - Cons is done with its cell again. Checks if the next cell is free
  - It is used by cons, then it reuses the same cell contents

So each cell has a defined lifetime, thus can have resources allocated to it via a specific allocator.
Each cell can be in the state:
  - Wip: The producer is writing to the cell. It cannot be read from the consumer, whom must wait for the producer to finish
  - Ready: The Produces is done writing the cell. It can be read from the consumer. The producer cannot override the cell until the consumer is done reading 
  - Read: The consumer is reading the cell. The producer cannot override the cell until the consumer is done reading
  - Empty: The consumer is done reading the cell. The producer can write onto it

Each thread is just a cell:
  |--------------------|
  | frame agnostic data|
  | (cannot be read    |
  | from other frames) |
  |--------------------|
  | cell | cell | cell |
  |--------------------|
