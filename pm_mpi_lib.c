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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <mpi.h>
#include "pm_mpi_lib.h"


#define MAX_FPATH_LEN 128
#define MAX_FLINE_LEN 128


static const char ver[] = "4.0.0";

static const char sys_pm_cnt_dir[] = "/sys/cray/pm_counters/";

// Supported power management counters
typedef enum pm_counters
{
    PM_COUNTER_FRESHNESS = 0,  // The freshness counter MUST be first in the list
    PM_COUNTER_POWER,
    PM_COUNTER_ENERGY,
    PM_COUNTER_ACCEL_POWER,
    PM_COUNTER_ACCEL_ENERGY,
    PM_COUNTER_STARTUP,
    PM_COUNTER_POWER_CAP,
    PM_COUNTER_ACCEL_POWER_CAP,
    PM_NCOUNTERS
} pm_counter_e ;

static const char* cnt_fname[] = {
  "freshness", /* The freshness counter MUST be first in the list */
  "power",
  "energy",
  "accel_power",
  "accel_energy",
  "startup",
  "power_cap",
  "accel_power_cap"
};

static int rank = -1;
static int min_node_rank = 0;
static int monitor_cnt = 0;
static int non_monitor_cnt = 0;
static int first_record = 0;
static int mpi_comm_monitor = 0;

static FILE* cnt_fp[PM_NCOUNTERS];
static FILE* log_fp = NULL;
static double tm0 = 0.0;
static double entot0 = 0.0;
static int last_nstep = 0;
static long int init_startup = 0;

static int all_initialised = 0;


		
int pm_is_accelerator_counter(const unsigned int i) {
  return (i == PM_COUNTER_ACCEL_POWER ||
          i == PM_COUNTER_ACCEL_ENERGY ||
  	  i == PM_COUNTER_ACCEL_POWER_CAP);
}
		
// return the first line from the counter file identified by i
void pm_get_first_line(const unsigned int i, char* line, const unsigned int len) {
  if (NULL != line) {
    memset(line, 0, len);
    if (i < PM_NCOUNTERS && NULL != cnt_fp[i]) {
      rewind(cnt_fp[i]);
      while (NULL != fgets(line, len, cnt_fp[i]) || EAGAIN == errno);	
    }
    else {
      strcpy(line, "0");
    }
  }
}
		
long int pm_get_counter_value(const unsigned int i) {
  char line[MAX_FLINE_LEN];
  pm_get_first_line(i, line, MAX_FLINE_LEN);
  return strtol(line, NULL, 10);
}

// determine the number of the node on which the process is running
int pm_get_node_number(void) {
  int node_name_len, nn_i, nn_m;
  int node_num = 0;
  char node_name[MPI_MAX_PROCESSOR_NAME];
    
  MPI_Get_processor_name(node_name, &node_name_len);
  if (node_name_len > 0) {
    nn_i = node_name_len-1;
    nn_m = 1;
    node_num = 0;
    while (nn_i > 0 && 0 != isdigit(node_name[nn_i])) {
      node_num = node_num + (node_name[nn_i]-'0')*nn_m;
      nn_m = 10*nn_m;
      nn_i = nn_i - 1;
    }
  }

  return node_num;
}

void pm_open_counter_files(void) {
    char cnt_fpath[MAX_FPATH_LEN];
    unsigned int max_fname_len = MAX_FPATH_LEN - strlen(sys_pm_cnt_dir);

    strcpy(cnt_fpath, sys_pm_cnt_dir);
    strncat(cnt_fpath, cnt_fname[PM_COUNTER_FRESHNESS], max_fname_len);
    		
    // always open the freshness counter file first
    cnt_fp[PM_COUNTER_FRESHNESS] = fopen(cnt_fpath, "r");
    if (NULL == cnt_fp[PM_COUNTER_FRESHNESS]) {
      fprintf(stderr, "pm_mpi_lib: failed to open %s!\n", cnt_fpath);
    }
    else {  	
      // if the freshness counter has been opened successfully
      // attempt to open the other counter files		
      for (int i = 0; i < PM_NCOUNTERS; i++) {		
        if (PM_COUNTER_FRESHNESS == i) {
  	  continue;
  	}
  					
  	strcpy(cnt_fpath, sys_pm_cnt_dir);
    	strncat(cnt_fpath, cnt_fname[i], max_fname_len);

	cnt_fp[i] = fopen(cnt_fpath, "r");
    	if (NULL == cnt_fp[i] && 0 == pm_is_accelerator_counter(i)) {
      	  fprintf(stderr, "pm_mpi_lib: failed to open %s!\n", cnt_fpath);
      	}			
      }
    }
}

void pm_close_counter_files(void) {
  for (int i = 0; i < PM_NCOUNTERS; i++) {
    if (NULL != cnt_fp[i]) {
      fclose(cnt_fp[i]);
      cnt_fp[i] = NULL;
    }
  }
}

// return true if pm_mpi_open has been called successfully
int pm_mpi_ok(void) {
  int ok = 0;
  
  if (-1 != rank) {
    if (min_node_rank == rank) {
      ok = (monitor_cnt > 0);
      ok = (0 != ok && NULL != cnt_fp[PM_COUNTER_FRESHNESS]);
      ok = (0 != ok && NULL != cnt_fp[PM_COUNTER_POWER]);
      ok = (0 != ok && NULL != cnt_fp[PM_COUNTER_ENERGY]);
      				
      if (0 == rank) {
        ok = (0 != ok && NULL != log_fp);
      }
    }
    else {
      ok = (non_monitor_cnt > 0);
    }
  }

  return ok;
}



// rank zero opens the output file
// allow the calling rank to self-identify as a monitoring process
// each monitoring process obtains the number of monitors
// call pm_mpi_monitor(-1,1)
void pm_mpi_open(const char* log_fpath) {  
  int node_num = 0;
  MPI_Comm mpi_comm_node;
  
  if (0 != pm_mpi_ok()) {
      return;
  }
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
  if (0 == rank) {
    if (NULL != log_fp) {
      fclose(log_fp);
      log_fp = NULL;
    }
    // open power management counter log file
    if (NULL == log_fp) {
      log_fp = fopen(log_fpath, "w"); 
    }
    if (NULL == log_fp) {
      log_fp = fopen("./pm_log.out", "w");
    }
  }

  // determine the number of the node running this process
  node_num = pm_get_node_number();
      
  // determine if this rank is the minimum rank for the identified node number
  MPI_Comm_split(MPI_COMM_WORLD, node_num, rank, &mpi_comm_node);
  MPI_Allreduce(&rank, &min_node_rank, 1, MPI_INTEGER, MPI_MIN, mpi_comm_node);
 
  if (rank == min_node_rank) {
    // the minimum rank on a node is responsible for monitoring
    // the performance counters on that node  
    MPI_Comm_split(MPI_COMM_WORLD, 1, rank, &mpi_comm_monitor);
  }
  else {
    MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &mpi_comm_monitor);
  }
  
  // ensure all monitor ranks have self-identified...
  MPI_Barrier(MPI_COMM_WORLD);
  // ...before each monitor rank obtains the total number of monitors
  if (min_node_rank == rank) {
    MPI_Comm_size(mpi_comm_monitor, &monitor_cnt);
  }
  else {
    // and other processes obtain the number of non-monitors
    MPI_Comm_size(mpi_comm_monitor, &non_monitor_cnt);
  }
  
  if (min_node_rank == rank) {
    pm_open_counter_files();
    init_startup = pm_get_counter_value(PM_COUNTER_STARTUP);
  }
  
  int ok = pm_mpi_ok();
  int all_ok = 0;
  MPI_Allreduce(&ok, &all_ok, 1, MPI_INTEGER, MPI_MIN, MPI_COMM_WORLD);
  all_initialised = all_ok;
  if (0 != all_initialised) {
    // do initial record, which ends with MPI_Barrier
    first_record = 1;
    pm_mpi_monitor(-1, 1);
  }
  else {
    pm_mpi_close();
  }
  			
} // end of pm_mpi_open() function



// read counter values if first rank on node,
// and output those values if rank zero
void pm_mpi_monitor(const int nstep, const int sstep) {
  
  if (0 == all_initialised) {
    return;
  }

  MPI_Barrier(MPI_COMM_WORLD);
    
  if (min_node_rank == rank) {
    long int start_freshness, end_freshness;
    double pmc_energy, tot_pmc_energy;
    long int pmc_power, tot_pmc_power;
    
    // get time
    double tm = MPI_Wtime();
    if (0 != first_record) {
      tm0 = tm;
      first_record = 0;
    }
    
    // read the point-in-time power and accumulated energy counters
    int fresh = 0;
    while (0 == fresh) {
      start_freshness = pm_get_counter_value(PM_COUNTER_FRESHNESS);
      pmc_power = pm_get_counter_value(PM_COUNTER_POWER);
      pmc_energy = pm_get_counter_value(PM_COUNTER_ENERGY);
      end_freshness = pm_get_counter_value(PM_COUNTER_FRESHNESS);
      fresh = (end_freshness == start_freshness);
    }
  
    MPI_Reduce(&pmc_power, &tot_pmc_power, 1, MPI_LONG, MPI_SUM, 0, mpi_comm_monitor);
    MPI_Reduce(&pmc_energy, &tot_pmc_energy, 1, MPI_DOUBLE, MPI_SUM, 0, mpi_comm_monitor);
         		
    // output data
    if (0 == rank) {
      if (tm0 == tm) {
        // this function is being called by pm_mpi_open()
        entot0 = tot_pmc_energy;
        
        if (NULL != log_fp) {
          fprintf(log_fp, "pm_mpi_lib v%s: time (s), step, substep, average power (W), energy used (J)\n", ver);
        }
      }

      tot_pmc_energy = round(tot_pmc_energy);
      double avg_pmc_power = (monitor_cnt > 0) ? ((double) tot_pmc_power)/((double) monitor_cnt) : 0.0;
      double dif_pmc_energy = tot_pmc_energy - entot0;
        
      if (NULL != log_fp) {   
        // update counter data file   
        fprintf(log_fp, "%f %d %d %f %f\n", tm-tm0, nstep, sstep, avg_pmc_power, dif_pmc_energy); 
      }
    }
  } // end of <if (min_node_rank == rank)> clause
  
  last_nstep = nstep;
  
  MPI_Barrier(MPI_COMM_WORLD); 
  
} // end of pm_mpi_monitor() function



// close the file used to record counter data
void pm_mpi_close() {

  if (0 != all_initialised) {
    // do the last record
    pm_mpi_monitor(last_nstep+1, 1);
  }
  
  // if monitoring process (i.e., first process on node)    
  if (min_node_rank == rank) {
    long int final_startup = pm_get_counter_value(PM_COUNTER_STARTUP);
    if (final_startup != init_startup) {
      fprintf(stderr, "pm_mpi_lib meaurements invalid! Blade-controller was restarted for node %d.\n", pm_get_node_number());
    }
	
    pm_close_counter_files();
        
    if (0 == rank && NULL != log_fp) {
      // close performance counter data file
      fclose(log_fp);
      log_fp = NULL;
    }
  }	
  
  all_initialised = 0;
  MPI_Barrier(MPI_COMM_WORLD);
  
} // end of pm_mpi_close() function
