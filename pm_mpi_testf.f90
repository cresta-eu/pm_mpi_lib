! Copyright (c) 2023 The University of Edinburgh

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
 
  integer :: ierr, i, res
  integer :: comm, rank
  character (len=14) :: pmc_out_fn = "./pmc/log.out"//CHAR(0)
  
  call MPI_Init(ierr)
  
  call pm_mpi_initialise(pmc_out_fn)

  do i=1,10
    res = pm_mpi_record(1,i,1,1)
    call sleep(1)
  end do

  call pm_mpi_finalise()

  call MPI_Finalize(ierr)
 
end program pm_testf

