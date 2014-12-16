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

static char ver[]="2.0.0";

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
static FILE *fpmc = NULL;
static double first_time = 0.0;
static unsigned long long int first_energy = 0;
static int last_nstep = 0;


void pm_init_counters(int n,pm_counter_e *counters){
  int i,k;
  int nc;
  int opened_freshness=0;
  char file[MAXFLEN];

  if (n==0) {
    nc=PM_NCOUNTERS;
   } else {
    nc=n;
   }
 
  for(i=0;i<nc;i++){

    if (n==0) {
      k=i;
     } else {
      k=counters[i];
     }
    if (k==PM_COUNTER_FRESHNESS) opened_freshness=1;
    strcpy(file,syspmcdir);
    strcat(file,"/");
    strncat(file,fname[k],MAXFLEN);

    if ( (fps[k]=fopen(file,"r"))!=NULL){
      //             fprintf(stderr,"pm_lib(pm_init): Opened %s\n",file);
      nopen++;
     } else {
      if (! (k==PM_COUNTER_ACCEL_POWER || 
             k==PM_COUNTER_ACCEL_ENERGY ||
             k==PM_COUNTER_ACCEL_POWER_CAP) ) {
       fprintf(stderr,"pm_lib(pm_init): Failed to open file %s\n",file);
       }
     }
  }

  if (! opened_freshness){
    k=PM_COUNTER_FRESHNESS;
    strcpy(file,syspmcdir);
    strcat(file,"/");
    strncat(file,fname[k],MAXFLEN);
    if ( (fps[k]=fopen(file,"r"))!=NULL){
      nopen++;
      //      fprintf(stderr,"pm_lib(pm_init): had to open %s\n",file);
     } else {
       fprintf(stderr,"pm_lib(pm_init): Failed to open file %s\n",file);
     }

  }

}

void pm_init(){
  pm_init_counters(0,NULL);
}

void pm_close(){
  int k;
  for(k=0;k<PM_NCOUNTERS;k++) {
    if (fps[k]!=NULL) {
      fclose(fps[k]);
    }
  }
}

// Return the number of coutner files that are currently open
int pm_get_num_opencounters(){
  return nopen;
}

// Return first line from file into line
// Do not alter line if file is not already open
static void fgetfirstline(char *line,int size,FILE *fp){
  
  if (fp==NULL) return;

  rewind(fp);
  while(fgets(line,size,fp)!=NULL || errno ==EAGAIN);
}

char* pm_get_counter_label(pm_counter_e counter){

  return fname[counter];

}

int pm_get_freshness(){
  int freshness;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_FRESHNESS]);
  freshness=atoi(input_line);
  
  return freshness;

}

int pm_get_power(){
  int power;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_POWER]);
  power=atoi(input_line);
  
  return power;

}

int pm_get_accel_power(){
  int accel_power;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_ACCEL_POWER]);
  accel_power=atoi(input_line);
  
  return accel_power;

}

int pm_get_power_cap(){
  int power_cap;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_POWER_CAP]);
  power_cap=atoi(input_line);
  
  return power_cap;

}

int pm_get_accel_power_cap(){
  int accel_power_cap;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_ACCEL_POWER_CAP]);
  accel_power_cap=atoi(input_line);
  
  return accel_power_cap;

}

unsigned long long int pm_get_energy(){
  unsigned long long int energy;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_ENERGY]);
  energy=strtoull(input_line,NULL,10);
  
  return energy;

}

unsigned long long int pm_get_accel_energy(){
  unsigned long long int accel_energy;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_ACCEL_ENERGY]);
  accel_energy=strtoull(input_line,NULL,10);
  
  return accel_energy;

}

unsigned long long int pm_get_startup(){
  unsigned long long int startup;
  char input_line[MAXLLEN];

  fgetfirstline(input_line,MAXLLEN,fps[PM_COUNTER_STARTUP]);
  startup = strtoull(input_line,NULL,10);
  
  return startup;

}

int pm_mpi_ok() {
  int ok = 0;
  
  if (-1 != rank) {
    if (0 == rank) {
      ok = (NULL != fpmc && first_time > 0.0 && monitor_cnt > 0);
    }
    else {
      if (min_node_rank == rank) {
        ok = (monitor_cnt > 0);
      }
      else {
        ok = 1;
      }
    }
  }
  
  return ok;
}

// opens performance counter data file if rank zero
// allow the calling rank to self-identify as a monitoring process
// obtain the number of other monitors
void pm_mpi_open(char* pmc_out_fn) {
  
  int node_name_len, nn_i, nn_m, node_num;
  char node_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Comm mpi_comm_node;
  MPI_Comm mpi_comm_monitor;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
  if (0 == rank) {
    if (NULL != fpmc) {
      fclose(fpmc);
      fpmc = NULL;
    }
    // open performance counter data file
    if (NULL != pmc_out_fn) {
      fpmc = fopen(pmc_out_fn, "w"); 
    }
    if (NULL == fpmc) {
      fpmc = fopen("./PMC", "w");
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
    // also open the power counter files
    pm_init();
  }
  
  if (0 == rank) {
    // prepare to get elapsed time
    first_time = MPI_Wtime();
  }
  
  // ensure that first_energy is initialised
  pm_mpi_monitor(0);
}


// read and record performance counters if first rank on node
void pm_mpi_monitor(int nstep) {
  
  int good_freshness, start_freshness, end_freshness;
  unsigned long long int pmc_energy, tot_pmc_energy;
  int pmc_power, tot_pmc_power;
  double avg_pmc_power;
  
  if (!pm_mpi_ok()) {
    pm_mpi_open(NULL);
    if (!pm_mpi_ok()) {
      return;
    }
  }
  
  if (min_node_rank == rank) {
    
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
    
  }
  else {
    pmc_power = 0;
    pmc_energy = 0;  
  }
    
  MPI_Allreduce(&pmc_power, &tot_pmc_power, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(&pmc_energy, &tot_pmc_energy, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
         
  if (0 == rank) {
        
    if (0 == first_energy) {
      // this function is being called by pm_mpi_open
      first_energy = tot_pmc_energy;
      if (NULL != fpmc) {
        fprintf(fpmc, "pm_mpi_lib v%s: time (s), step, average power (W), energy used (J)\n", ver);
      }
    }
    else {
      avg_pmc_power = 0.0;
      if (monitor_cnt > 0) {
        avg_pmc_power = ((double) tot_pmc_power)/((double) monitor_cnt);
      }
      
      if (NULL != fpmc) { 
        // update performance counter data file   
        fprintf(fpmc, "%f %ld %f %ld\n", MPI_Wtime()-first_time, nstep, avg_pmc_power, tot_pmc_energy-first_energy);
      }
    }
    
  }
  
  last_nstep = nstep;
  
}


// close the file used to record performance counter data
void pm_mpi_close() {

  // do the last monitoring call
  pm_mpi_monitor(last_nstep+1);
  
  if (min_node_rank == rank) {
    // close the power counter files
    pm_close();
  }
  
  if (0 == rank) {
    if (NULL != fpmc) {
      // close performance counter data file
      fclose(fpmc);
      fpmc = NULL;
    }
  }
  
}
