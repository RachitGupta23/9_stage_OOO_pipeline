# 9-Stage OOO Pipeline
This is a C++ based model that implements a 9-stage Out of Order Issue Pipeline.

## Overview
- The 9 stages are: FETCH, DECODE, RENAME, REG_READ, DISPATCH, ISSUE, EXECUTE, WRITEBACK, RETIRE.
- The model assumes perfect branch prediction and perfect memory access.
- It models for three types of instructions:
   - 0 - EXE latency of 1
   - 1 - EXE latency of 2
   - 2 - EXE latency of 5
- The simulator reads the trace file, with each line having 1 instruction, defining the instruction type, dst register and 2 src registers. If it does not use a dst or src register, it is replaced with a "-".
- The trace also includes the PC to maintain correct order during retire.
- Each stage is implemented as a function call.
- The Issue Queue (IQ), Rename Map Table (RMT) and Reorder Buffer (ROB) are implemented as class objects.
- Does not model actual register values and data as the purpose is to model the performance when varying the IQ size, ROB size and Fetch bundle size.

## Features
- The simulator accepts 4 parameters:
   - ROB size - how many entries are possible in the ROB.
   - IQ size - number of entries in the Issue Queue.
   - Width - number of instructions fetched togther in the fetch stage.
   - trace file - location of input trace file.
- The model outputs when an instruction enters each stage and how much time it spends in each stage. This shows how much time it took to execute each instruction and where it stalled if at all.
- We use a while loop to run each stage and each iteration of the while loop is equivalent to 1 clock cycle.
- With this we then calculate the IPC of the model with the given parameters and trace file.

# Instructions
1. Type `make` to build.  (Type `make clean` first if you already compiled and want to recompile from scratch.)
2. To run simulator with params, ROB size = 256, IQ size = 32, Width = 4, then run `./sim 256 32 4 gcc_trace.txt`
