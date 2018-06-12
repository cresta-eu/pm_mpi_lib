pm_mpi_lib
==========

This repository holds source code for the pm_mpi_lib library. There are also two small test harnesses that demonstrate how to call the library functions from Fortran and C codes. Furthermore, the test folder contains readme files that explain exactly how to integrate the pm_mpi_lib source with the DL_POLY (v4.05) and CP2K (v2.6.14482) source codes. Run make to build the pm_mpi_lib object and module (*.mod) files. The make also builds executables for two test harnesses written in C and Fortran.
When building for gnu and intel environments run make CFLAGS_EXTRA="-std=c99".

The following text describes the interface provided by the three functions of the pm_mpi_lib library.

void pm_mpi_initialise(const char* out_fn)

The parameter, out_fn, points to a null-terminated string that specifies the name of the file that will hold the PM counter data: a NULL parameter value will set the output file name to ./PMC. The open function also calls pm_mpi_record(-1,1,1,1) in order to determine a baseline for the cumulative energy. In addition, rank 0 establishes a temporal baseline by calling MPI_Wtime and also writes a one-line header to the output file, which gives the library version followed by the names of the data items that will appear in the subsequent lines.


void pm_mpi_finalise(void)

This function calls pm_mpi_record(nstep+1,1,1,0), see below. All counter files are closed, then rank 0 closes the output file.


void pm_mpi_record(const int nstep, const int sstep, const int initial_sync, const int initial_rec)

The first two parameters (nstep and sstep) allow the client to label each set of counter values that are output by rank 0.
If initial_sync is true MPI_Barrier is called before reading takes place.
If initial_sync and initial_rec are both true then the energy counters are read before and after the initial barrier.
Note, initial_rec is only used when initial_sync is true.

The output file contains lines of space-separated fields. A description of each field follows (the C-language data type is given in square brackets).

Time [double]: the time as measured by MPI_Wtime (called by rank zero) that has elapsed since the last call to pm_mpi_open.
Step [int]: a simple numerical label: e.g., the iteration count, assuming pm_mpi_monitor is being called from within a loop.
Sub-step [int]: another numerical label that might be required if there is more than one monitor call within the same loop.
Point-in-time power [double]: the average power reading across all assigned compute nodes.
Energy [unsigned long long int]: the energy used by all assigned compute nodes since the last call to pm_mpi_open.
