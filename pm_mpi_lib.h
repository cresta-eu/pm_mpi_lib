/*
 Copyright (c) 2014 Harvey Richardson, Michael Bareford
 All rights reserved.

 See the LICENSE file elsewhere in this distribution for the
 terms under which this software is made available.

*/

#ifndef PM_MPI_LIB_H
#define PM_MPI_LIB_H

extern void pm_mpi_open(char* pmc_out_fn);
extern void pm_mpi_monitor(int nstep);
extern void pm_mpi_close();

#endif
