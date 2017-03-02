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

module pm_mpi_lib
  implicit none

  public
  
  interface
   subroutine pm_mpi_open(out_fn) bind(c,name='pm_mpi_open')
    use, intrinsic :: iso_c_binding
    character(c_char) :: out_fn(*)
   end subroutine pm_mpi_open
  end interface

  interface
   subroutine pm_mpi_monitor(nstep,sstep) bind(c,name='pm_mpi_monitor')
    use, intrinsic :: iso_c_binding
    integer(c_int),value :: nstep
    integer(c_int),value :: sstep
   end subroutine pm_mpi_monitor
  end interface
  
  interface
   subroutine pm_mpi_close() bind(c,name='pm_mpi_close')
    use, intrinsic :: iso_c_binding
   end subroutine pm_mpi_close
  end interface
     

end module pm_mpi_lib
