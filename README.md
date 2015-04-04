pm_mpi_lib
==========

This repository holds source code for the pm_mpi_lib library. There are also two small test harnesses that demonstrate how to call the library functions from Fortran and C codes. Furthermore, the test folder contains readme files that explain exactly how to integrate the pm_mpi_lib source with the DL_POLY (v4.05) and CP2K (v2.6.14482) source codes. Run make to build the pm_mpi_lib object and module (*.mod) files. The make also builds executables for two test harnesses written in C and Fortran.

The following text describes the interface provided by the three functions of the pm_mpi_lib library.

void pm_mpi_open(char* pmc_out_fn)

The parameter, pmc_out_fn, points to a null-terminated string that specifies the name of the file that will hold the PM counter data: a NULL parameter value will set the output file name to ./PMC. The open function also calls pm_mpi_monitor(0) in order to determine a baseline for the cumulative energy. In addition, rank 0 establishes a temporal baseline by calling MPI_Wtime and also writes a one-line header to the output file, which gives the library version followed by the names of the data items that will appear in the subsequent lines.

void pm_mpi_close(void)

This function calls pm_mpi_monitor(nstep+1). All counter files are closed, then rank 0 closes the output file.

void pm_mpi_monitor(int nstep)

The parameter, nstep, allows the client to label each set of counter values that are output by rank 0. The output file contains lines of space-separated fields. A description of each field follows (the C-language data type is given in square brackets).

Time [double]: the time as measured by MPI_Wtime (called by rank zero) that has elapsed since the last call to pm_mpi_open.
Step [int]: a simple numerical label: e.g., the iteration count, assuming pm_mpi_monitor is being called from within a loop.
Point-in-time power [double]: the average power reading across all assigned compute nodes.
Energy [unsigned long long int]: the energy used by all assigned compute nodes since the last call to pm_mpi_open.
