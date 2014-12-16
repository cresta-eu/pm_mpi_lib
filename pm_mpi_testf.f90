! Copyright (c) 2014 Harvey Richardson, Michael Bareford
! All rights reserved.
!
! See the LICENSE file elsewhere in this distribution for the
! terms under which this software is made available.

program pm_testf
  use mpi
  use pm_mpi_lib
  
  implicit none
 
  integer :: ierr, i
  integer :: comm, rank
  character (len=13) :: pmc_out_fn = "pmc_test.out"//CHAR(0)

  call MPI_Init(ierr)
  
  call pm_mpi_open(pmc_out_fn)

  do i=1,10
    call pm_mpi_monitor(i)
    call sleep(1)
  end do

  call pm_mpi_close()

  call MPI_Finalize(ierr)
 
end program pm_testf

