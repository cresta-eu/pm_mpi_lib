version=1.0

FC=ftn
CC=cc

all: pm_mpi_lib.o pm_mpi_test pm_mpi_testf

pm_mpi_lib.o: pm_mpi_lib.c pm_mpi_lib.h
	$(CC) $(CFLAGS) -c pm_mpi_lib.c

pm_mpi_lib_interface.o: pm_mpi_lib_interface.f90
	$(FC) -c pm_mpi_lib_interface.f90

pm_mpi_testf: pm_mpi_testf.f90 pm_mpi_lib_interface.o pm_mpi_lib.o
	$(FC) -o pm_mpi_testf pm_mpi_testf.f90 pm_mpi_lib_interface.o pm_mpi_lib.o
	
pm_mpi_test: pm_mpi_test.c pm_mpi_lib.o pm_mpi_lib.h
	$(CC) $(CFLAGS) -o pm_mpi_test pm_mpi_test.c pm_mpi_lib.o
	
clean: 
	$(RM) *.o *.mod pm_mpi_test pm_mpi_testf


