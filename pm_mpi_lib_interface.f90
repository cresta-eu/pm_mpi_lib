! Copyright (c) 2014 Harvey Richardson, Michael Bareford
! All rights reserved.
!
! See the LICENSE file elsewhere in this distribution for the
! terms under which this software is made available.

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
