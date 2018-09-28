## Procsim

Procsim is a simple processor simulator written in C implementing variable numbers of functional units for different instructions, ROB size, and amount of physical registers availalbe to the processor. 

## Usage

Use the provided Makefile to compile procsim properly, then run as the following template:

    ./procsim [OPTIONS] < [trace file]

OPTIONS can be any number of the following and is also described with the -h help option.
    -j number of k0 functional units
    -k number of k1 functional units
    -l number of k2 functional units
    -f number of instructions to dispatch
    -r number of entries in the ROB
    -p number of physical registers to utilize

Procsim has default values for k0, k1, k2, f, r, and p (all found in procsim.h), but will need a trace file to read or it will stall, a few of which are provided in the traces directory. For example, to run the gcc trace with default settings, use:
    
    ./procsim < traces/gcc.100k.trace

To change the number of k1 functional units to use:

    ./procsim -k 1 < traces/gcc.100k.trace

This will use default settings except for k1, which will be 1 instead of 2.

Procsim will output each value it is using in the simulation for verification.

## Results

Procsim will output execution statistics after a successful simulation. It will print the total number of instructions executed (either limited by the trace file or the -f option), the average number of instructions fired and retired per cycle, and the total run time in cycles.
