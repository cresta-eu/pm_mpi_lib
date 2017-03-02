! Copyright (c) 2017 The University of Edinburgh

! Licensed under the Apache License, Version 2.0 (the "License");
! you may not use this file except in compliance with the License.
! You may obtain a copy of the License at

! http://www.apache.org/licenses/LICENSE-2.0

! Unless required by applicable law or agreed to in writing, software
! distributed under the License is distributed on an "AS IS" BASIS,
! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
! See the License for the specific language governing permissions and
! limitations under the License.

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
    call pm_mpi_monitor(i,1)
    call sleep(1)
  end do

  call pm_mpi_close()

  call MPI_Finalize(ierr)
 
end program pm_testf

