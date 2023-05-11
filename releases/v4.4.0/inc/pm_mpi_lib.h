/* 
  Copyright (c) 2023 The University of Edinburgh.

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

// Supported power management counters
typedef enum pm_counters
{
    PM_COUNTER_FRESHNESS = 0,  // The freshness counter MUST be first in the list
    PM_COUNTER_POWER,
    PM_COUNTER_ENERGY,
    PM_COUNTER_CPU_POWER,
    PM_COUNTER_CPU_ENERGY,
    PM_COUNTER_MEMORY_POWER,
    PM_COUNTER_MEMORY_ENERGY,
    PM_COUNTER_CPU0_TEMP,
    PM_COUNTER_CPU1_TEMP,
    PM_COUNTER_ACCEL_POWER,
    PM_COUNTER_ACCEL_ENERGY,
    PM_COUNTER_STARTUP,
    PM_COUNTER_POWER_CAP,
    PM_COUNTER_ACCEL_POWER_CAP,
    PM_COUNTER_RAW_SCAN_HZ,
    PM_NCOUNTERS
} pm_counter_e ;

extern void pm_mpi_initialise(const char* out_fn);
extern unsigned int pm_mpi_record(const int nstep, const int sstep, const int initial_sync, const int initial_rec);
extern void pm_mpi_finalise();

#endif
