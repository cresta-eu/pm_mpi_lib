pm_mpi_lib
==========

This repository holds source code for the `pm_mpi_lib` library. There are also two small test harnesses that demonstrate how to call the library functions from Fortran and C codes.
Furthermore, the test folder contains readme files that explain exactly how to integrate the `pm_mpi_lib` source with the CP2K 2023.1 source code. Run make to build the `pm_mpi_lib`
object and module (`*.mod`) files. The make also builds executables for two test harnesses written in C and Fortran.

The following text describes the interface provided by the three functions of the `pm_mpi_lib` library.

```bash
void pm_mpi_initialise(const char* out_fn)
```

The parameter, `out_fn`, points to a null-terminated string that specifies the name of the log file that will hold the PM counter data, e.g., `${SLURM_SUBMIT_DIR}/pmc/log.out`. It is
preferable to have the log file in a subfolder of the submission directory. That way, the subfolder can be created with the appropriate Lustre file striping options that will be
subsequently applied to the log file. The initialise function determines which ranks will act as *monitors*. As each (water-cooled) compute node has its own PM counter files, there is
one monitor rank per (water-ccoled) node. The monitor rank is responsible for reading the PM files and writing that data to the log file.

The initialise function also calls `pm_mpi_record(-1,1,1,1)` in order to determine a baseline for the cumulative energy.

Since the log file has multiple writers, it is written in binary via calls to `MPI_File_write_ordered`. There exists therefore a Python script for converting the binary log file
to ASCII, called `make_ascii_log_file.py`, which takes two arguments, the path to the binary log file and the number of fields stored per log line - that number is 12.


```bash
void pm_mpi_finalise(void)
```

The finalise function calls `pm_mpi_record(nstep,1,1,0)` (described below). All counter files are closed, then the monitor ranks close the log file.

```bash
void pm_mpi_record(const int nstep, const int sstep, const int initial_sync, const int initial_rec)
```

The first two parameters (`nstep` and `sstep`) allow the client to label each set of counter values that are output by each monitor rank.<br>
If initial_sync is true `MPI_Barrier` is called before reading takes place.<br>
If `initial_sync` and `initial_rec` are both true then the energy counters are read before and after the initial barrier.<br>
Note, `initial_rec` is only used when `initial_sync` is true.

The binary output file contains a list of log lines, where each log line is a sequence of twelve 8-byte binary fields. A description of each of these fields is given below.

**Rank**: the monitor rank id.<br>
**Time**: the time as measured by `MPI_Wtime`.<br>
**Step**: a simple numerical label: e.g., the iteration count, assuming `pm_mpi_record` is being called from within a loop.<br>
**Sub-step**: another numerical label that might be required if there is more than one monitor call within the same loop.<br>
**Point-in-time node power [W]**: the node power reading for a particular compute node.<br>
**Point-in-time power used by the CPU domain [W]**: the cpu domain power reading for a particular compute node.<br>
**Point-in-time power used by the memory domain [W]**: the memory domain power reading for a particular compute node.<br>
**Accumulated node energy [J]**: the node energy used by a particular compute node.<br>
**Accumulated energy used by the CPU domain [J]**: the cpu domain energy used by a particular compute node.<br>
**Accumulated energy used by the memory domain [J]**: the memory domain energy used by a particular compute node.<br>
**Temperature Energy for the CPU domain on socket 0 [Celsius]**: the temperature reading for the cpu domain on socket 0.<br>
**Temperature Energy for the CPU domain on socket 1 [Celsius]**: the temperature reading for the cpu domain on socket 1.<br>

Lastly, the Slurm output file records which MPI rank (or monitor id) is responsible for which compute node (identified by `nid` number)..

```bash
pm_mpi_lib: rank 256 detected water-cooled node 5311.
pm_mpi_lib: rank 384 detected water-cooled node 5312.
pm_mpi_lib: rank 128 detected water-cooled node 5305.
pm_mpi_lib: rank 0 detected water-cooled node 5301.
```
