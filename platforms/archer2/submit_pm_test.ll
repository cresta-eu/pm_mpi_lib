#!/bin/bash --login
  
#SBATCH --job-name=pm
#SBATCH --nodes=4
#SBATCH --tasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --time=00:20:00
#SBATCH --account=z19
#SBATCH --partition=standard
#SBATCH --qos=short


module -q restore
module -q load cpe/21.09
module -q load PrgEnv-cray

export LD_LIBRARY_PATH=${CRAY_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}


export OMP_NUM_THREADS=1

mkdir -p ${SLURM_SUBMIT_DIR}/pmc
lfs setstripe -c -1 ${SLURM_SUBMIT_DIR}/pmc

srun --distribution=block:block --hint=nomultithread --unbuffered ./pm_mpi_test 
