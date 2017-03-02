# Copyright (c) 2017 The University of Edinburgh

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FC=ftn
CC=cc

ifneq ($(PE_ENV),CRAY)
  CFLAGS_EXTRA = -std=c99
endif

CFLAGS = -fPIC $(CFLAGS_EXTRA)

all: pm_mpi_lib.o libpmmpi.a libpmmpi.so pm_mpi_test pm_mpi_testf

pm_mpi_lib.o: pm_mpi_lib.c pm_mpi_lib.h
	$(CC) $(CFLAGS) -c pm_mpi_lib.c

libpmmpi.a: pm_mpi_lib.o
	ar rcs $@ $^

libpmmpi.so: pm_mpi_lib.o
	$(CC) -fPIC -shared -o libpmmpi.so pm_mpi_lib.o

pm_mpi_lib_interface.o: pm_mpi_lib_interface.f90
	$(FC) -c pm_mpi_lib_interface.f90

pm_mpi_testf: pm_mpi_testf.f90 pm_mpi_lib_interface.o pm_mpi_lib.o
	$(FC) -o pm_mpi_testf pm_mpi_testf.f90 pm_mpi_lib_interface.o pm_mpi_lib.o

pm_mpi_test: pm_mpi_test.c pm_mpi_lib.o pm_mpi_lib.h
	$(CC) $(CFLAGS) -o pm_mpi_test pm_mpi_test.c pm_mpi_lib.o

clean: 
	$(RM) *.o *.mod *.out *.a *.so pm_mpi_test pm_mpi_testf


