/* 
  Copyright (c) 2017 The University of Edinburgh.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef PM_MPI_LIB_H
#define PM_MPI_LIB_H

extern void pm_mpi_open(const char* out_fn);
extern void pm_mpi_monitor(const int nstep, const int sstep);
extern void pm_mpi_close();

#endif
