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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <mpi.h>
#include "pm_mpi_lib.h"


static const char ver[] = "5.0.0";

#define PM_RECORD_OK                 0
#define PM_RECORD_UNINITIALISED      1
#define PM_RECORD_BLADE_RESTART      2
#define PM_RECORD_COUNTER_FILE_ERROR 3

#define MAX_FPATH_LEN 128
#define MAX_FLINE_LEN 128

#define PMC_CNTR_CNT    8
#define PMC_FIELD_CNT  12


static const char sys_pmc_dir[] = "/sys/cray/pm_counters/";
static const char* cntr_fname[] = {
  "freshness", /* The freshness counter MUST be first in the list */
  "power",
  "energy",
  "cpu_power",
  "cpu_energy",
  "memory_power",
  "memory_energy",
  "cpu0_temp",
  "cpu1_temp",
  "accel_power",
  "accel_energy",
  "startup",
  "power_cap",
  "accel_power_cap",
  "raw_scan_hz"
};

static int rank = -1;

static int min_node_rank = 0;
static int monitor_cnt = 0;
static int non_monitor_cnt = 0;
static int mpi_comm_monitor = 0;

static int node_is_water_cooled = 0;

static FILE* cntr_fp[PM_NCOUNTERS];
static int last_nstep = 0;
static long int init_startup = 0;

static int pmc_index[PMC_CNTR_CNT] =
    { PM_COUNTER_POWER, PM_COUNTER_CPU_POWER, PM_COUNTER_MEMORY_POWER,
      PM_COUNTER_ENERGY, PM_COUNTER_CPU_ENERGY, PM_COUNTER_MEMORY_ENERGY,
      PM_COUNTER_CPU0_TEMP, PM_COUNTER_CPU1_TEMP };

static int mpi_err = 0;
static MPI_File log_fh = 0;
static MPI_Status mpi_stat;

static int all_initialised = 0;

static int system_error = 0;


int pm_is_accelerator_counter(const unsigned int i) {
  return (i == PM_COUNTER_ACCEL_POWER ||
          i == PM_COUNTER_ACCEL_ENERGY ||
  	  i == PM_COUNTER_ACCEL_POWER_CAP);
}


int pm_is_node_water_cooled(int node_num) {
    struct stat sb;
    int water_cooled = 0;

    if (stat(sys_pmc_dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        water_cooled = 1;
    }

    return water_cooled;
}


void pm_open_counter_files(void) {
    char cntr_fpath[MAX_FPATH_LEN];
    unsigned int max_fname_len = MAX_FPATH_LEN - strlen(sys_pmc_dir);

    strcpy(cntr_fpath, sys_pmc_dir);
    strncat(cntr_fpath, cntr_fname[PM_COUNTER_FRESHNESS], max_fname_len);
    		
    // always open the freshness counter file first
    cntr_fp[PM_COUNTER_FRESHNESS] = fopen(cntr_fpath, "r");
    if (NULL == cntr_fp[PM_COUNTER_FRESHNESS]) {
        fprintf(stderr, "pm_mpi_lib: failed to open %s!\n", cntr_fpath);
    }
    else {  	
        // if the freshness counter has been opened successfully
        // attempt to open the other counter files		
        for (int i = 0; i < PM_NCOUNTERS; i++) {		
            if (PM_COUNTER_FRESHNESS == i) {
  	        continue;
  	    }
  					
  	    strcpy(cntr_fpath, sys_pmc_dir);
    	    strncat(cntr_fpath, cntr_fname[i], max_fname_len);

	    cntr_fp[i] = fopen(cntr_fpath, "r");
    	    if (NULL == cntr_fp[i] && 0 == pm_is_accelerator_counter(i)) {
      	        fprintf(stderr, "pm_mpi_lib: failed to open %s!\n", cntr_fpath);
      	    }			
        }
    }
}


void pm_close_counter_files(void) {
    for (int i = 0; i < PM_NCOUNTERS; i++) {
        if (NULL != cntr_fp[i]) {
            fclose(cntr_fp[i]);
            cntr_fp[i] = NULL;
        }
    }
}


// return, via the line parameter, the first line from the counter file identified by i
// this method returns zero if counter file has been read successfully, otherwise the system
// error (errno) is returned
int pm_get_first_line(const unsigned int i, char* line, const unsigned int len) {
    int syserr = 0;

    memset(line, 0, len);
    if (i < PM_NCOUNTERS && NULL != cntr_fp[i]) {
        rewind(cntr_fp[i]);
  	    
        do {
            fgets(line, len, cntr_fp[i]);

    	    if (feof(cntr_fp[i])) {
	        // end of file reached,
	        // assume the last len characters have been read
	        break;
	    }     
	    else if (ferror(cntr_fp[i])) {
	        if (EAGAIN != errno) {
	            // error not due to file being busy
	            // record error, zero line buffer and return
	            syserr = errno;
		    fprintf(stderr, "pm_mpi_lib: pm_get_first_line failed for PM counter %d: errno = %d.\n", i, errno);
		    strcpy(line, "0");
		    break;
		}
	    }
		
	} while (1);
    }
    else {
        syserr = ENOENT;
        strcpy(line, "0");
    }
    
    return syserr;
}


long int pm_get_counter_value(const unsigned int i) {
    char line[MAX_FLINE_LEN];
    long int val = 0;
    	
    int syserr = pm_get_first_line(i, line, MAX_FLINE_LEN);   
    if (0 == syserr) {
        val = strtol(line, NULL, 10);
    }
    else {
        system_error = syserr;
    }
            
    return val;
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


// return true if pm_mpi_initialise has been called successfully
int pm_mpi_ok(void) {
    int ok = 0;
  
    if (-1 != rank) {
        if (min_node_rank == rank && 0 != node_is_water_cooled) {
            ok = (monitor_cnt > 0 && 0 != log_fh);

	    if (0 != node_is_water_cooled) {
                ok = (0 != ok && NULL != cntr_fp[PM_COUNTER_FRESHNESS]);
                ok = (0 != ok && NULL != cntr_fp[PM_COUNTER_STARTUP]);

		for (int i=0; i<PMC_CNTR_CNT; i++) {
		    ok = (0 != ok && NULL != cntr_fp[pmc_index[i]]);
		}
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
// call pm_mpi_record(-1,1,1,0)
void pm_mpi_initialise(const char* log_fpath) {  
    int node_num = 0;
    MPI_Comm mpi_comm_node;
  
    if (0 != pm_mpi_ok()) {
        return;
    }
  
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // determine the number of the node running this process
    node_num = pm_get_node_number();

    // determine if this rank is the minimum rank for the identified node number
    MPI_Comm_split(MPI_COMM_WORLD, node_num, rank, &mpi_comm_node);
    MPI_Allreduce(&rank, &min_node_rank, 1, MPI_INTEGER, MPI_MIN, mpi_comm_node);
 
    if (rank == min_node_rank) {
	// the minimum rank on a node is responsible for monitoring
        // the pm counters on that node...
	
        node_is_water_cooled = pm_is_node_water_cooled(node_num);

	if (0 != node_is_water_cooled) {
            // ...but pm counters are only available if node is water cooled
            MPI_Comm_split(MPI_COMM_WORLD, 1, rank, &mpi_comm_monitor);
	    fprintf(stderr, "pm_mpi_lib: rank %d detected water-cooled node %d.\n", rank, node_num);
	}
	else {
	    MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &mpi_comm_monitor);
	    fprintf(stderr, "pm_mpi_lib: rank %d detected air-cooled node %d!\n", rank, node_num);
	}
    }
    else {
        MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &mpi_comm_monitor);
    }
  
    // ensure all monitor ranks have self-identified...
    MPI_Barrier(MPI_COMM_WORLD);
    // ...before each monitor rank obtains the total number of monitors
    if (min_node_rank == rank && 0 != node_is_water_cooled) {
        MPI_Comm_size(mpi_comm_monitor, &monitor_cnt);
    }
    else {
        // and other processes obtain the number of non-monitors
        MPI_Comm_size(mpi_comm_monitor, &non_monitor_cnt);
    }

    system_error = 0;
    if (min_node_rank == rank && 0 != node_is_water_cooled) {
        pm_open_counter_files();

        init_startup = pm_get_counter_value(PM_COUNTER_STARTUP);
	
        // open power management counter log file
	mpi_err = MPI_File_open(mpi_comm_monitor, log_fpath,
                                MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_APPEND,
                                MPI_INFO_NULL, &log_fh);
            
        if (mpi_err != MPI_SUCCESS) {
            fprintf(stderr, "pm_mpi_lib: MPI_File_open failed for rank %d: err = %d.\n", rank, mpi_err);
	    log_fh = 0;
	    mpi_err = 0;
	}
    }
  
    int ok = pm_mpi_ok() && (0 == system_error);
    int all_ok = 0;
    MPI_Allreduce(&ok, &all_ok, 1, MPI_INTEGER, MPI_MIN, MPI_COMM_WORLD);
    all_initialised = all_ok;
    if (0 != all_initialised) {
        // do initial record, which ends with MPI_Barrier
        pm_mpi_record(-1, 1, 1, 0);
    }
    else {
        pm_mpi_finalise();
    }
  			
} // end of pm_mpi_initialise() function



// read counter values if first rank on node,
// and output those values if rank zero
unsigned int pm_mpi_read_counter_values(const int nstep, const int sstep) {
  
    if (min_node_rank != rank || 0 == node_is_water_cooled) {
        return PM_RECORD_OK;
    }

    double pmc_data[PMC_FIELD_CNT] =
        { (double) rank, MPI_Wtime(), (double) nstep, (double) sstep,
	  0.0, 0.0, 0.0,
          0.0, 0.0, 0.0,
          0.0, 0.0 };

    long int start_freshness, end_freshness;
    
    // read the point-in-time power and temperature and accumulated energy counters
    int fresh = 0;
    long int current_startup = 0;
    while (0 == fresh) {
        start_freshness = pm_get_counter_value(PM_COUNTER_FRESHNESS);
        
	for (int i=0; i<PMC_CNTR_CNT; i++) {
	    pmc_data[4+i] = (double) pm_get_counter_value(pmc_index[i]);
        }

	current_startup = pm_get_counter_value(PM_COUNTER_STARTUP);
	end_freshness   = pm_get_counter_value(PM_COUNTER_FRESHNESS);
        
	fresh = (end_freshness == start_freshness);

	if (0 != system_error) {
	    system_error = 0;
	    return PM_RECORD_COUNTER_FILE_ERROR;  
	}
    }

    if (current_startup != init_startup) {
        fprintf(stderr, "pm_mpi_lib meaurements invalid! Blade-controller was restarted for node %d.\n", pm_get_node_number());
	return PM_RECORD_BLADE_RESTART;
    }
    
    if (0 != log_fh) {   
        // write to counter log file
	mpi_err = MPI_File_write_ordered(log_fh, pmc_data, PMC_FIELD_CNT, MPI_DOUBLE_PRECISION, &mpi_stat);
	if (mpi_err != MPI_SUCCESS) {
            fprintf(stderr, "pm_mpi_lib: MPI_File_write_ordered failed for rank %d: err = %d.\n", rank, mpi_err);
            log_fh = 0;
	    mpi_err = 0;
        }
    } 
  
    return PM_RECORD_OK;
  
} // end of pm_mpi_read_counter_values() function


// read and record counter values if first rank on node
// the reading will be labelled with the step and substep numbers
// if initial_sync is true MPI_Barrier is called before reading takes place
// if initial_sync and initial_rec are true then counters are read before and after initial barrier
// initial_rec is only used when initial_sync is true
unsigned int pm_mpi_record(const int nstep, const int sstep, const int initial_sync, const int initial_rec) {
   
    if (0 == all_initialised) {
        return PM_RECORD_UNINITIALISED;
    }

    unsigned int res = PM_RECORD_OK;
    if (0 != initial_sync) {
        if (0 != initial_rec) {
	    res = pm_mpi_read_counter_values(nstep, sstep);
	}
	
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (PM_RECORD_OK == res) {
        res = pm_mpi_read_counter_values(nstep, sstep);
    }
    
    last_nstep = nstep;
  
    MPI_Barrier(MPI_COMM_WORLD);

    return res;
  
} // end of pm_mpi_record() function



// close the files used to read and record counter data
void pm_mpi_finalise() {

    if (0 != all_initialised) {
        // do the last record
        pm_mpi_record(last_nstep+1, 1, 1, 0);
    }
  
    // if monitoring process (i.e., first process on water-cooled node)    
    if (min_node_rank == rank && 0 != node_is_water_cooled) {
        pm_close_counter_files();
        
        if (0 != log_fh) {
            // close pm counter log file
	    MPI_File_close(&log_fh);
            log_fh = 0;
        }
    }	

    system_error = 0;  
    all_initialised = 0;
    MPI_Barrier(MPI_COMM_WORLD);
  
} // end of pm_mpi_finalise() function
