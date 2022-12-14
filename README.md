# Building histograms with MPI
An implementation of a simple parallelized program that calculates and visualizes the character counts for all letters in a given text file using MPI.

### mpi_cc.c does the following:
- Reads the text file into an array of chars.
- The character array is loaded on the main process (pid=0) and then shared with the rest of the processes. Each process will compute the character count for a section of the character array.
- Once all processes have finished calculating, their character counts are merged using MPI on the main process.

## How to run mpi_cc.c:
1. Compile by utilizing the make tool provided by the Makefile: `make` or `make mpi_cc`
- Note: mpi_cc.c will be executed with mpiexec
2. Execute mpi_cc.c with the following command line arguments: `mpiexec -n [num_nodes] ./mpi_cc [-l] [-s] [filename]`
  - Since mpi_cc.c is executed using mpiexe, we must execute the program using mpiexec followed by -n and the number of MPI nodes you want executing mpi_cc.
- `-l` is an optional flag indicating the output should be a letter count histogram. If this flag is not provided, simple number counts are displayed.
- `-s` is an optional flag indicating the histogram must be saved to a text file called `out.txt` in addition to displaying the histogram to stdout. If this flag is not provided, output is not saved.
- `filename` is the name of the file you want to analyze.
