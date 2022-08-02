The mod file in ./obj/ARCHER/psmp/ is the gnu version of pm_mpi_lib. The path to this file specifies where within the cp2k directory the file needs to 
be copied. For this example only one source file is altered, namely, ./src/motion/md_run.F, which again needs to be copied to the appropriate area 
within the cp2k directory.

The following commands are required to build cp2k so that it is based on mixed mode MPI/OpenMP.

module restore
module swap PrgEnv-cray PrgEnv-gnu
module load fftw

make -j 16 ARCH=ARCHER2 VERSION=psmp

The executable is written to ./exe/ARCHER2/cp2k.psmp and must be copied to the work area together with the necessary input data (see ./data/) before
a cp2k job can be submitted.
