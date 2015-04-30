/*

 Copyright (c) 2014 Harvey Richardson, Michael Bareford
 All rights reserved.

 See the LICENSE file elsewhere in this distribution for the
 terms under which this software is made available.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <mpi.h>
#include "pm_mpi_lib.h"


// Counters supported
typedef enum pm_counters {
    PM_COUNTER_FRESHNESS,  /* The freshness counter MUST be first in the list */
    PM_COUNTER_POWER,
    PM_COUNTER_ENERGY,
    PM_COUNTER_ACCEL_POWER,
    PM_COUNTER_ACCEL_ENERGY,
    PM_COUNTER_STARTUP,
    PM_COUNTER_POWER_CAP,
    PM_COUNTER_ACCEL_POWER_CAP,
    PM_NCOUNTERS
} pm_counter_e ;
  

#define MAXFLEN 100
#define MAXLLEN 100

static char ver[]="3.1.0";

static char *fname[]={
  "freshness",
  "power",
  "energy",
  "accel_power",
  "accel_energy",
  "startup",
  "power_cap",
  "accel_power_cap"};


static char syspmcdir[]="/sys/cray/pm_counters";
static int nopen=0;  // Number of counter files open
static FILE *fps[PM_NCOUNTERS];

static int rank = -1;
static int min_node_rank = 0;
static int monitor_cnt = 0;
static int non_monitor_cnt = 0;
static MPI_Comm mpi_comm_monitor;
static FILE *fout = NULL;
static int first_monitor = 1;
static double tm0 = 0.0;
static unsigned long long int entot0 = 0;
static int last_nstep = 0;

static int open = 0;



void pm_init_counters(int n, pm_counter_e *counters) {
  int i,k;
  int nc;
  int opened_freshness=0;
  char file[MAXFLEN];

  if (n==0) {
    nc=PM_NCOUNTERS;
  }
  else {
    nc=n;
  }
 
  for (i=0; i<nc; i++) {
    if (n == 0) {
      k = i;
    }
    else {
      k = counters[i];
    }
    
    if (k == PM_COUNTER_FRESHNESS) {
      opened_freshness = 1;
    }
    
    strcpy(file, syspmcdir);
    strcat(file, "/");
    strncat(file, fname[k], MAXFLEN);

    if ((fps[k] = fopen(file,"r")) != NULL) {
      // fprintf(stderr,"pm_lib(pm_init): Opened %s\n",file);
      nopen++;
    }
    else {
      if (!(k == PM_COUNTER_ACCEL_POWER || 
            k == PM_COUNTER_ACCEL_ENERGY ||
            k == PM_COUNTER_ACCEL_POWER_CAP)) {
        fprintf(stderr,"pm_lib(pm_init): Failed to open file %s\n",file);
      }
    }
  }

  if (!opened_freshness) {
    k = PM_COUNTER_FRESHNESS;
    strcpy(file, syspmcdir);
    strcat(file, "/");
    strncat(file, fname[k], MAXFLEN);
    if ((fps[k] = fopen(file,"r")) != NULL) {
      nopen++;
      //fprintf(stderr,"pm_lib(pm_init): had to open %s\n",file);
    }
    else {
      fprintf(stderr,"pm_lib(pm_init): Failed to open file %s\n",file);
    }
  }

}

void pm_init() {
  pm_init_counters(0, NULL);
}

void pm_close() {
  for(int k = 0; k < PM_NCOUNTERS; k++) {
    if (fps[k] != NULL) {
      fclose(fps[k]);
    }
  }
}

// Return the number of coutner files that are currently open
int pm_get_num_opencounters() {
  return nopen;
}

// Return first line from file into line
// Do not alter line if file is not already open
static void fgetfirstline(char *line, int size, FILE *fp) {
  
  if (fp==NULL) {
    return;
  }
  
  rewind(fp);
  while(fgets(line,size,fp) != NULL || errno == EAGAIN);
}

char* pm_get_counter_label(pm_counter_e counter) {
  return fname[counter];
}

int pm_get_freshness() {
  int freshness;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_FRESHNESS]);
  freshness=atoi(input_line);
  
  return freshness;
}

int pm_get_power() {
  int power;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_POWER]);
  power=atoi(input_line);
  
  return power;
}

int pm_get_accel_power() {
  int accel_power;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_ACCEL_POWER]);
  accel_power=atoi(input_line);
  
  return accel_power;
}

int pm_get_power_cap() {
  int power_cap;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_POWER_CAP]);
  power_cap=atoi(input_line);
  
  return power_cap;
}

int pm_get_accel_power_cap() {
  int accel_power_cap;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_ACCEL_POWER_CAP]);
  accel_power_cap = atoi(input_line);
  
  return accel_power_cap;
}

unsigned long long int pm_get_energy() {
  unsigned long long int energy;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_ENERGY]);
  energy=strtoull(input_line, NULL, 10);
  
  return energy;
}

unsigned long long int pm_get_accel_energy() {
  unsigned long long int accel_energy;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_ACCEL_ENERGY]);
  accel_energy=strtoull(input_line, NULL, 10);
  
  return accel_energy;
}

unsigned long long int pm_get_startup() {
  unsigned long long int startup;
  char input_line[MAXLLEN];

  fgetfirstline(input_line, MAXLLEN, fps[PM_COUNTER_STARTUP]);
  startup = strtoull(input_line, NULL, 10);
  
  return startup;
}



// return 1 if pm_mpi_open has been called successfully
int pm_mpi_ok() {
  int ok = 0;
  
  if (-1 != rank) {
  
    if (min_node_rank == rank) {
      ok = (monitor_cnt > 0 && pm_get_num_opencounters() > 0);
      
      if (0 == rank) {
        ok = (ok && fout);
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
void pm_mpi_open(char* out_fn) {
  
  int node_name_len, nn_i, nn_m, node_num;
  char node_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Comm mpi_comm_node;
  
  
  if (0 != open) {
    return;
  }
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
  if (0 == rank) {
    if (fout) {
      fclose(fout);
      fout = NULL;
    }
    // open performance counter data file
    if (out_fn) {
      fout = fopen(out_fn, "w"); 
    }
    if (!fout) {
      fout = fopen("./pmc.out", "w");
    }
  }

  // determine the number of the node this rank has been assigned
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
    // open the power counter files
    pm_init();
  }
  
  
  int ok = pm_mpi_ok(), all_ok = 0;
  MPI_Allreduce(&ok, &all_ok, 1, MPI_INTEGER, MPI_MIN, MPI_COMM_WORLD);
  open = all_ok;
  if (0 == open) {
    pm_mpi_close();
  }
  else {
    // do initial monitoring call, which ends with MPI_Barrier
    first_monitor = 1;
    pm_mpi_monitor(-1, 1);
  }
} // end of pm_mpi_open() function



// read counter values if first rank on node,
// and output those values if rank zero
void pm_mpi_monitor(int nstep, int sstep) {
   
  if (0 == open) {
    return;
  }
  
  if (min_node_rank == rank) {
    int good_freshness, start_freshness, end_freshness;
    unsigned long long int pmc_energy, tot_pmc_energy;
    int pmc_power, tot_pmc_power;
    
    // get time
    double tm = MPI_Wtime();
    if (1 == first_monitor) {
      tm0 = tm;
      first_monitor = 0;
    }
    
    // read the point-in-time power and accumulated energy counters
    good_freshness = 0;
    while (0 == good_freshness) {
      start_freshness = pm_get_freshness();
      pmc_power = pm_get_power();
      pmc_energy = pm_get_energy();
      end_freshness = pm_get_freshness();
      if (end_freshness == start_freshness) {
        good_freshness = 1;
      }
    }
  
    MPI_Allreduce(&pmc_power, &tot_pmc_power, 1, MPI_LONG, MPI_SUM, mpi_comm_monitor);
    MPI_Allreduce(&pmc_energy, &tot_pmc_energy, 1, MPI_LONG_LONG, MPI_SUM, mpi_comm_monitor);
         
    // output data
    if (0 == rank) {
      if (tm0 == tm) {
        // this function is being called by pm_mpi_open()
        entot0 = tot_pmc_energy;
        
        if (fout) {
          fprintf(fout, "pm_mpi_lib v%s: time (s), step, substep, average power (W), energy used (J)\n", ver);
        }
      }
    
      double avg_pmc_power = (monitor_cnt > 0) ? ((double) tot_pmc_power)/((double) monitor_cnt) : 0.0;
        
      if (fout) {   
        // update counter data file   
        fprintf(fout, "%f %d %d %f %ld\n", tm-tm0, nstep, sstep, avg_pmc_power, tot_pmc_energy-entot0); 
      }
    }
    
  } // end of <if (min_node_rank == rank)> clause
  
  last_nstep = nstep;
  
  MPI_Barrier(MPI_COMM_WORLD);
  
} // end of pm_mpi_monitor() function



// close the file used to record counter data
void pm_mpi_close() {

  if (1 == open) {
    // do the last monitoring call
    pm_mpi_monitor(last_nstep+1, 1);
  }
  
  // if monitoring process (i.e., first process on node)    
  if (min_node_rank == rank) {
  
    if (0 == rank) {
      if (fout) {
        // close performance counter data file
        fclose(fout);
        fout = NULL;
      }
    }
    
  } 
  
  open = 0;
  MPI_Barrier(MPI_COMM_WORLD);
  
} // end of pm_mpi_close() function
