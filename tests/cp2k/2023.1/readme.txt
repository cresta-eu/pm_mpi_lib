The mod file in ./obj/ARCHER/psmp/ is the gnu version of pm_mpi_lib. The path to this file specifies where within the cp2k directory the file needs to 
be copied. For this example only one source file is altered, namely, ./src/motion/md_run.F, which again needs to be copied to the appropriate area 
within the cp2k directory.

The following commands are required to build cp2k so that it is based on mixed mode MPI/OpenMP.

module -q restore
module -q load PrgEnv-gnu
module -q load cray-fftw
module -q load cray-python
module -q load cpe/21.09
module -q load mkl

cd ./libs
git clone https://github.com/cresta-eu/pm_mpi_lib.git
cd ..

cp ./src/motion/md_run.F ./src/motion/md_run.F.std
cp ./libs/pm_mpi_lib/tests/cp2k/2023.1/src/motion/md_run.F ./src/motion/md_run.F.md
cp ./src/motion/md_run.F.pm ./src/motion/md_run.F
cp ./libs/pm_mpi_lib/tests/cp2k/2023.1/arch/ARCHER2.psmp.pm ./arch/

make -j 8 ARCH=ARCHER2 VERSION=psmp.pm

The executable is written to ./exe/ARCHER2/cp2k.psmp.pm and must be copied to the work area together with the necessary input data (see ./data/) before
a cp2k job can be submitted.
