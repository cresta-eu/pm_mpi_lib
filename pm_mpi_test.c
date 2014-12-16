/*

 Copyright (c) 2014 Harvey Richardson, Michael Bareford
 All rights reserved.

 See the LICENSE file elsewhere in this distribution for the
 terms under which this software is made available.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include "pm_mpi_lib.h"

int main(int argc,char **argv){
  
  int i, ierr, rank, comm;
  
  MPI_Init(NULL, NULL);
  
  pm_mpi_open("./pmc_test.out");
  
  for (i=1; i<10; i++) {
    pm_mpi_monitor(i);
    sleep(500000);
  }
  
  pm_mpi_close();
  
  MPI_Finalize();

  return EXIT_SUCCESS;

}

