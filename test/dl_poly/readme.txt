Three files within the ./source folder have been altered in order for dl_poly to make use of the pm_mpi_lib library. These are dl_poly.f90, ./VV/w_md_vv.f90 
and Makefile. These three files together with the pm_mpi_lib source code (i.e., pm_mpi_lib.h, pm_mpi_lib.c and pm_mpi_lib_interface.f90) need to be 
copied to the appropriate areas within the dl_poly directory - the pm_mpi_lib files can be copied to the ./source folder.

The dl_poly executable can be built for all three programming environments supported by ARCHER.

make archer2-cray BINPATH="${HOME/home/work}/dl_poly/archer-cray/job01"
make archer2-gnu BINPATH="${HOME/home/work}/dl_poly/archer-gnu/job01"
make archer2-aocc BINPATH="${HOME/home/work}/dl_poly/archer-aocc/job01"

The executables are written to specific locations within the work area. The paths indicated by BINPATH must exist before attempting to compile dl_poly. 
In addition, the input data files (see ./data/) must be copied to the work area.
